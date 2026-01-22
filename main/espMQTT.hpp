#pragma once
#include "config.hpp"
#include "tools.hpp"

#include "espIO.hpp"
#include "esp_ota_ops.h"
#include "esp_tls.h"
#include "mqtt_client.h"

namespace mesp {

    using std::string;

    using callback_fun_ptr = void (*)(esp_mqtt_client_handle_t, const string&);

    inline std::atomic<int> time_out = 0;//@_@超时处理,之后优化一下


    struct Mqttclient {
      private:
        static void mqtt_event_callback(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
            // fpr("\nMqtt事件分发");
            esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);
            esp_mqtt_client_handle_t client = event->client;
            // int msg_id = -1;

            Mqttclient& This = *static_cast<Mqttclient*>(handler_args);
            callback_fun_ptr f = This.event_data_fun;

            switch (esp_mqtt_event_id_t(event_id)) {
            case MQTT_EVENT_CONNECTED:
                if (This.state == mqtt_state::disconnected || This.state == mqtt_state::error) {
                    fpr("MQTT_EVENT_CONNECTED（MQTT重连成功）");
                } else {
                    fpr("MQTT_EVENT_CONNECTED（MQTT连接成功）");
                }
                fpr("MQTT会话已建立");
                This.state = mqtt_state::connected;
                This.state.notify_all();
                break;
            case MQTT_EVENT_DISCONNECTED:
                fpr("MQTT_EVENT_DISCONNECTED（MQTT断开连接）");
                if (event->error_handle) {
                    fpr("断开原因 - 错误类型: ", event->error_handle->error_type);
                    if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                        fpr("TCP传输错误，将自动重连");
                    } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                        fpr("连接被拒绝，返回码: ", event->error_handle->connect_return_code);
                    }
                } else {
                    fpr("正常断开或网络异常，将自动重连");
                }
                This.state = mqtt_state::disconnected;
                This.state.notify_all();
                break;
            case MQTT_EVENT_BEFORE_CONNECT:
                fpr("MQTT_EVENT_BEFORE_CONNECT（MQTT连接前，正在尝试连接...）");
                break;
            case MQTT_EVENT_SUBSCRIBED:
                fpr("MQTT_EVENT_SUBSCRIBED（MQTT订阅成功），msg_id=", event->msg_id);
                if (event->topic_len > 0) {
                    fpr("订阅主题: ", string(event->topic, event->topic_len));
                }
                break;
            case MQTT_EVENT_UNSUBSCRIBED:
                fpr("MQTT_EVENT_UNSUBSCRIBED（MQTT取消订阅成功），msg_id=", event->msg_id);
                break;
            case MQTT_EVENT_PUBLISHED:
                fpr("MQTT_EVENT_PUBLISHED（MQTT消息发布成功），msg_id=", event->msg_id);
                break;
            case MQTT_EVENT_DATA:
                // fpr("MQTT_EVENT_DATA（接收到MQTT消息）");
                if (event->topic_len > 0) {
                    fpr("收到消息，主题: ", string(event->topic, event->topic_len), ", 数据长度: ", event->data_len);
                }
                fpr(string(event->data, event->data_len));
                time_out = 0;               
                f(client, string(event->data, event->data_len));
                break;
            case MQTT_EVENT_ERROR:
                fpr("MQTT_EVENT_ERROR（MQTT事件错误）");
                if (event->error_handle) {
                    if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                        fpr("TCP传输错误 - esp-tls错误代码：", event->error_handle->esp_tls_last_esp_err);
                        fpr("TLS堆栈错误号：", event->error_handle->esp_tls_stack_err);
                        fpr("Socket错误：", event->error_handle->esp_transport_sock_errno,
                            " (", strerror(event->error_handle->esp_transport_sock_errno), ")");
                        fpr("将自动尝试重连");
                    } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                        fpr("连接被拒绝，返回码：", event->error_handle->connect_return_code);
                        fpr("可能原因：用户名/密码错误、服务器拒绝连接等");
                    } else {
                        fpr("未知的错误类型：", event->error_handle->error_type);
                    }
                }
                This.state = mqtt_state::error;
                This.state.notify_all();
                break;
            default:
                fpr("其他MQTT事件id:", event->event_id);
                break;
            }
        }// mqtt_event_callback
      public:
        esp_mqtt_client_handle_t client = nullptr;
        const callback_fun_ptr event_data_fun;

        struct mqtt_state {
            constexpr static int init = 0;
            constexpr static int destroy = 1;
            constexpr static int connected = 2;
            constexpr static int disconnected = 3;
            constexpr static int error = 4;
        };// mqtt_state

        std::atomic<int> state = mqtt_state::init;

  Mqttclient(const string& server, const string& user, const string& pass, callback_fun_ptr f)
    : event_data_fun(f) 
{
    esp_mqtt_client_config_t mqtt_cfg{};
    mqtt_cfg.broker.address.uri = server.c_str();
    mqtt_cfg.broker.verification.skip_cert_common_name_check = true;
    mqtt_cfg.credentials.username = user.c_str();
    mqtt_cfg.credentials.authentication.password = pass.c_str();

    // 会话配置
    mqtt_cfg.session.keepalive = 60;              // 心跳间隔60秒（Keep-Alive）
    mqtt_cfg.session.disable_clean_session = false; // 启用clean session
    mqtt_cfg.session.protocol_ver = MQTT_PROTOCOL_V_3_1_1; // 使用MQTT 3.1.1协议
    
    // 网络配置
    mqtt_cfg.network.reconnect_timeout_ms = 5000; // 5秒重连超时
    mqtt_cfg.network.timeout_ms = 10000;          // 10秒网络超时
    mqtt_cfg.network.disable_auto_reconnect = false; // 启用自动重连（默认启用）
    
    // 缓冲区配置
    mqtt_cfg.buffer.size = 4096;                  // 增大接收缓冲区
    mqtt_cfg.buffer.out_size = 2048;              // 发送缓冲区
    
    // 任务配置
    mqtt_cfg.task.stack_size = 6144;              // 增大任务栈
    mqtt_cfg.task.priority = 5;                   // 提高任务优先级

    fpr("初始化MQTT客户端，服务器: ", server);
    fpr("Keep-Alive心跳间隔: ", mqtt_cfg.session.keepalive, "秒");
    fpr("自动重连: 启用，重连超时: ", mqtt_cfg.network.reconnect_timeout_ms, "毫秒");

    client = esp_mqtt_client_init(&mqtt_cfg);
    error_check(esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, Mqttclient::mqtt_event_callback, this), "Mqtt注册事件失败");
    error_check(esp_mqtt_client_start(client), "Mqtt初始化失败");
}
        ~Mqttclient() {
            if (state != mqtt_state::init)
                error_check(esp_mqtt_client_stop(client));
            error_check(esp_mqtt_client_destroy(client));
        }

        Mqttclient(const Mqttclient&) = delete;
        Mqttclient& operator=(const Mqttclient&) = delete;
        Mqttclient(Mqttclient&& r) noexcept
            : event_data_fun(r.event_data_fun) {
            client = r.client;
            state.store(r.state);
            r.client = nullptr;
            r.state = mqtt_state::init;
        }
        Mqttclient& operator=(Mqttclient&&) noexcept = delete;
        // 先都delete,不然还要加引用计数

        operator esp_mqtt_client_handle_t&() noexcept {
            return client;
        }
        operator const esp_mqtt_client_handle_t&() const noexcept {
            return client;
        }

        // 订阅主题
        void subscribe(const string& topic, int Qos = 1) {
            esp_mqtt_client_subscribe(client, topic.c_str(), Qos);
        }

        void error_check(esp_err_t err, const string& msg = "MQTT错误:") {
            if (err != ESP_OK) {
                fpr(msg, err);
                state.store(mqtt_state::error);
                state.notify_all();
            }
        }

        // 等待状态变换
        void wait() {
            state.wait(mqtt_state::init);
        }

        // mqtt已连接
        bool connected() const noexcept {
            return state == mqtt_state::connected;
        }

    };// Mqttclinet


    //mqtt连接错误这边,应该是外部给一个错误处理的回调,然后在这个回调里改变mqtt_done@_@

}// mesp
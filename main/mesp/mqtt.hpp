/*
* Copyright (C) 2025-2026 by nccrrv
* SPDX-License-Identifier: AGPL-3.0-or-later
* 
* mqtt.hpp
* MQTT客户端封装
* 
* Example:
*   
*/

#pragma once

#include "../mstd/Exstring.hpp"

#include "wsValue.hpp"
#include <algorithm>
#include <mqtt_client.h>
#include <thread>




namespace mesp {
    using mstd::Exstring;

    struct Mqttclient {

        struct state {
            int value = 0;

            constexpr state() = default;
            constexpr state(int v) : value(v) {}

            constexpr static int disconnected = 0;
            constexpr static int before_connect = 1;
            constexpr static int connected = 2;
            // constexpr static int disconnected = 3;

            constexpr bool operator==(int v) const noexcept {
                return value == v;
            }
        };//state

        using string_type = Exstring<64>;//用的字符串类型

        constexpr static size_t data_cache_size = 12288;//12kb

        using data_cache_type = Exstring<data_cache_size>;

        string_type server_ip;
        string_type user;
        string_type password;
        string_type topic_subscribe;
        string_type topic_publish;


        static void def_data_event(const Mqttclient& client, const data_cache_type& data) {//默认的正常的消息处理
            fpr(data);
        };


        static void def_error_event(const Mqttclient& client, const Exstring<128>& msg) {//默认的错误处理
            fpr(msg);
        };

        using data_event_type = decltype(&def_data_event);
        using error_event_type = decltype(&def_error_event);

        data_event_type data_event = def_data_event;
        error_event_type error_event = def_error_event;

        esp_mqtt_client_handle_t client = nullptr;
        wsValue<Exstring<32>> mqtt_state{"mqtt_state", "连接断开"};
        //mqtt连接状态,这其实和ws耦合了,但是写起来确实方便@_@
        //想到了,把error_event改为state_event,然后传入值

        volatile bool is_connected_flag = false;


        Mqttclient() = default;
        Mqttclient(const string_type& server_ip,
                   const string_type& user,
                   const string_type& password)
            : server_ip(server_ip), user(user), password(password) {
        }
        ~Mqttclient() {
            if (client == nullptr)
                return;
            error_check(esp_mqtt_client_stop(client), "_stop ");
            error_check(esp_mqtt_client_destroy(client), "_destroy");
        }

      private:
        static void mqtt_event_callback(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
            esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);
            esp_mqtt_client_handle_t client = event->client;
            // int msg_id = -1;

            Mqttclient& This = *static_cast<Mqttclient*>(handler_args);
            // callback_fun_ptr f = This.event_data_fun;

            // fpr("Mqtt事件回调,事件id=", event_id);

            switch (esp_mqtt_event_id_t(event_id)) {
            case MQTT_EVENT_CONNECTED:
                fpr("MQTT_EVENT_CONNECTED（MQTT连接成功）");
                This.is_connected_flag = true;
                This.mqtt_state.set_value("连接成功");
                This.subscribe();// 自动订阅
                break;
            case MQTT_EVENT_DISCONNECTED:
                fpr("MQTT_EVENT_DISCONNECTED（MQTT断开连接）");
                This.is_connected_flag = false;
                This.mqtt_state.set_value("连接断开");
                break;
            case MQTT_EVENT_BEFORE_CONNECT:
                fpr("Mqtt连接前");
                This.mqtt_state.set_value("尝试连接");
                break;
            case MQTT_EVENT_SUBSCRIBED:
                fpr("MQTT_EVENT_SUBSCRIBED（MQTT订阅成功），msg_id=", event->msg_id);
                This.mqtt_state.set_value("订阅成功");
                break;
            case MQTT_EVENT_UNSUBSCRIBED:
                fpr("MQTT_EVENT_UNSUBSCRIBED（MQTT取消订阅成功），msg_id=", event->msg_id);
                break;
            case MQTT_EVENT_PUBLISHED:
                fpr("MQTT_EVENT_PUBLISHED（MQTT消息发布成功），msg_id=", event->msg_id);
                This.mqtt_state.set_value("消息发送成功");
                break;
            case MQTT_EVENT_DATA: {
                //fpr("MQTT_EVENT_DATA（接收到MQTT消息）");
                //printf("主题=%.*s\r\n",event->topic_len,event->topic);
                // printf("%.*s\r\n", event->data_len, event->data);

                static data_cache_type data_cache;

                // fpr("消息长度:", event->data_len);//mqtt的不以'\0'结尾,有点坑
                size_t copy_len = std::min(static_cast<size_t>(event->data_len), data_cache.max_size() - 1);
                memcpy(data_cache.data(), event->data, copy_len);
                data_cache[copy_len] = '\0';//确保结尾
                This.data_event(This, data_cache);

                break;
            }
            case MQTT_EVENT_ERROR:
                fpr("MQTT_EVENT_ERROR（MQTT事件错误）");
                if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                    fpr("从esp-tls报告的最后错误代码：", event->error_handle->esp_tls_last_esp_err);
                    fpr("TLS堆栈最后错误号：", event->error_handle->esp_tls_stack_err);
                    fpr("最后捕获的errno：", event->error_handle->esp_transport_sock_errno,
                        strerror(event->error_handle->esp_transport_sock_errno));

                    if (event->error_handle->esp_tls_last_esp_err == 32774) {
                        This.mqtt_state.set_value("找不到主机");//闪的太快都看不到
                        webfpr("找不到主机");
                    }

                } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                    fpr("连接被拒绝错误：", event->error_handle->connect_return_code);
                    switch (event->error_handle->connect_return_code) {
                    case esp_mqtt_connect_return_code_t::MQTT_CONNECTION_ACCEPTED://0
                        break;
                    case esp_mqtt_connect_return_code_t::MQTT_CONNECTION_REFUSE_PROTOCOL://1
                        This.mqtt_state.set_value("协议错误");
                        break;
                    case esp_mqtt_connect_return_code_t::MQTT_CONNECTION_REFUSE_ID_REJECTED://2
                        This.mqtt_state.set_value("客户端ID被拒绝");
                        break;
                    case esp_mqtt_connect_return_code_t::MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE://3
                        This.mqtt_state.set_value("服务器不可用");
                        break;
                    case esp_mqtt_connect_return_code_t::MQTT_CONNECTION_REFUSE_BAD_USERNAME://4
                        This.mqtt_state.set_value("用户名错误");
                        break;
                    case esp_mqtt_connect_return_code_t::MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED://5
                        This.mqtt_state.set_value("用户名或密码错误");
                        break;
                    }
                } else {
                    fpr("未知的错误类型：", event->error_handle->error_type);
                    This.mqtt_state.set_value("未知错误");
                }
                break;
            default:
                fpr("其他事件id:", event->event_id);
                break;
            }
        }

        static void error_check(esp_err_t err, const auto& msg = "") {
            if (err != ESP_OK) {
                fpr("Mqtt错误", err, msg);
            }
        }

      public:
        void connect() {
            esp_mqtt_client_config_t mqtt_cfg{};
            mqtt_cfg.broker.address.uri = server_ip.c_str();
            mqtt_cfg.broker.verification.skip_cert_common_name_check = true;
            mqtt_cfg.credentials.username = user.c_str();
            mqtt_cfg.credentials.authentication.password = password.c_str();


            // 关键优化配置
            mqtt_cfg.buffer.size = data_cache_size;// 增大接收缓冲区
            mqtt_cfg.buffer.out_size = 2048;// 发送缓冲区
            mqtt_cfg.network.reconnect_timeout_ms = 5000;// 5秒重连
            // mqtt_cfg.task.stack_size = 6144;// 增大任务栈
            mqtt_cfg.task.priority = 5;// 提高任务优先级

            if (client != nullptr) {
                fpr("Mqtt已存在连接,先停止");
                error_check(esp_mqtt_client_stop(client), "Mqtt停止失败");
                error_check(esp_mqtt_client_destroy(client), "Mqtt销毁失败");
                client = nullptr;
            }

            fpr("Mqtt开始连接");
            client = esp_mqtt_client_init(&mqtt_cfg);
            error_check(esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, Mqttclient::mqtt_event_callback, this), "Mqtt注册事件失败");
            error_check(esp_mqtt_client_start(client), "Mqtt初始化失败");
        }

        void connect(const Exstring<64>& server_ip,
                     const Exstring<64>& user,
                     const Exstring<64>& password) {
            this->server_ip = server_ip;
            this->user = user;
            this->password = password;
            connect();
        }

        // 订阅主题
        void subscribe(const string_type& topic, int Qos = 1) {
            esp_mqtt_client_subscribe(client, topic.c_str(), Qos);
        }

        void subscribe(int Qos = 1) {
            esp_mqtt_client_subscribe(client, topic_subscribe.c_str(), Qos);
        }


        void publish(const auto& msg) const {
            fpr("发送消息:", msg);
            int msg_id = esp_mqtt_client_publish(client, topic_publish.c_str(), Meta::get_c_str(msg), Meta::get_size(msg), 0, 0);

            if (msg_id < 0)
                fpr("发送失败");
            else
                fpr("发送成功,消息id=", msg_id);
            // mstd::delay(2s);//@_@这些延时还可以调
        }

        bool is_connected() const noexcept {
            return is_connected_flag;
        }

    };//Mqttclient


    //@brief WebSocket专用大消息打印
    inline void webfpr_lager(const Mqttclient::data_cache_type& str) {
        constexpr static size_t max_size = Mqttclient::data_cache_type::max_size() + 1024;

        mstd::fpr("wsmsg: ", str);
        static StaticJsonDocument<max_size> doc;
        doc.clear();
        doc["log"] = str.c_str();
        if (doc.overflowed())
            mstd::fpr("webfpr_lager json overflowed");


        static char buffer[max_size];
        size_t len = serializeJson(doc, buffer, max_size);
        ws_server.textAll(buffer, len);
    }



}//mesp
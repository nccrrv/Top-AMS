#pragma once
#include "tools.hpp"
#include "../config.hpp"

#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"
#include "espIO.hpp"

namespace esp {

    using callback_fun_ptr = void(*)(esp_mqtt_client_handle_t,const std::string&);

    template<callback_fun_ptr f>
    void mqtt_event_handler(void* handler_args,esp_event_base_t base,int32_t event_id,void* event_data) {
        auto TAG = "MQTT ";
        fpr("\n事件分发");
        esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);
        esp_mqtt_client_handle_t client = event->client;
        int msg_id = -1;
        switch (esp_mqtt_event_id_t(event_id)) {
        case MQTT_EVENT_CONNECTED:
            fpr(TAG,"MQTT_EVENT_CONNECTED（MQTT连接成功）");
            msg_id = esp_mqtt_client_subscribe(client,config::topic_subscribe.c_str(),1);//这里不应该从config读@_@
            fpr(TAG,"发送订阅成功，msg_id=",msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            fpr(TAG,"MQTT_EVENT_DISCONNECTED（MQTT断开连接）");
            break;
        case MQTT_EVENT_BEFORE_CONNECT:
            fpr("连接前");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            fpr(TAG,"MQTT_EVENT_SUBSCRIBED（MQTT订阅成功），msg_id=",event->msg_id);
            esp::gpio_out(esp::LED_L,true);
            mstd::delay(1s);
            esp::gpio_out(esp::LED_L,false);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            fpr(TAG,"MQTT_EVENT_UNSUBSCRIBED（MQTT取消订阅成功），msg_id=",event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            fpr(TAG,"MQTT_EVENT_PUBLISHED（MQTT消息发布成功），msg_id=",event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            // fpr(TAG,"MQTT_EVENT_DATA（接收到MQTT消息）");
            // printf("主题=%.*s\r\n",event->topic_len,event->topic);
            printf("%.*s\r\n",event->data_len,event->data);
            // fpr(std::string(event->data));

            //可以加约束,约束F分派
            f(client,std::string(event->data));
            break;
        case MQTT_EVENT_ERROR:
            fpr(TAG,"MQTT_EVENT_ERROR（MQTT事件错误）");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                fpr(TAG,"从esp-tls报告的最后错误代码：",event->error_handle->esp_tls_last_esp_err);
                fpr(TAG,"TLS堆栈最后错误号：",event->error_handle->esp_tls_stack_err);
                fpr(TAG,"最后捕获的errno：",event->error_handle->esp_transport_sock_errno,
                    strerror(event->error_handle->esp_transport_sock_errno));
            }
            else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                fpr(TAG,"连接被拒绝错误：",event->error_handle->connect_return_code);
            }
            else {
                fpr(TAG,"未知的错误类型：",event->error_handle->error_type);
            }
            break;
        default:
            fpr(TAG,"其他事件id:",event->event_id);
            break;
        }
    }//mqtt_event_handler


    template<callback_fun_ptr f>
    [[nodiscard]] esp_mqtt_client_handle_t mqtt_app_start(const std::string server,const std::string& user,const std::string& pass) {
        esp_mqtt_client_config_t mqtt_cfg{};
        mqtt_cfg.broker.address.uri = server.c_str();
        //mqtt_cfg.broker.verification.use_global_ca_store = false;
        mqtt_cfg.broker.verification.skip_cert_common_name_check = true;
        mqtt_cfg.credentials.username = user.c_str();
        mqtt_cfg.credentials.authentication.password = pass.c_str();

        fpr("[APP] Free memory: ",esp_get_free_heap_size(),"bytes");
        esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
        /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */

        esp_mqtt_client_register_event(client,MQTT_EVENT_ANY,mqtt_event_handler<f>,nullptr);
        esp_mqtt_client_start(client);
        return client;
    }

} // namespace esp

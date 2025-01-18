#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include "esp_system.h"
#include "esp_partition.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"
#include <sys/param.h>


#include "include/tools.hpp"
#include "include/espIO.hpp"
#include "include/espWIFI.hpp"
#include "include/bambu.hpp"
#include "include/ArduinoJson.hpp"

#if __has_include("config.hpp")
#include "config.hpp"
#else
#include "config_example.hpp"
#endif



int bed_target_temper_max = 0;
std::atomic<int> extruder = 1;// 1-16,初始通道默认为1
int sequence_id = -1;
//std::atomic<int> print_error = 0;
std::atomic<int> ams_status = -1;
std::atomic<bool> pause_lock{ false };//暂停锁

inline constexpr int 正常 = 0;
inline constexpr int 退料完成需要退线 = 260;
inline constexpr int 退料完成 = 0;//同正常
inline constexpr int 进料检查 = 262;
inline constexpr int 进料冲刷 = 263;//推测
inline constexpr int 进料完成 = 768;








void publish(esp_mqtt_client_handle_t client,const std::string& msg) {
	esp::gpio_out(esp::LED_L,true);
	fpr("发送消息:",msg);
	int msg_id = esp_mqtt_client_publish(client,config::topic_publish.c_str(),msg.c_str(),msg.size(),0,0);
	if (msg_id < 0)
		fpr("发送失败");
	else
		fpr("发送成功,消息id=",msg_id);
	// fpr(TAG, "binary sent with msg_id=%d", msg_id);
	esp::gpio_out(esp::LED_L,false);
	mstd::delay(2s);//@_@这些延时还可以调整看看
}

void callback_fun(esp_mqtt_client_handle_t client,const std::string& json) {//接受到信息的回调
	//fpr(json);
	using namespace ArduinoJson;
	JsonDocument doc;
	DeserializationError error = deserializeJson(doc,json);


	static int bed_target_temper = -1;
	bed_target_temper = doc["print"]["bed_target_temper"] | bed_target_temper;
	std::string gcode_state = doc["print"]["gcode_state"] | "unkonw";


	if (bed_target_temper > 0 && bed_target_temper < 17) {//读到的温度是通道
		if (gcode_state == "PAUSE") {
			// mstd::delay(2s);
			if (bed_target_temper_max > 0) {//似乎热床置零会导致热端固定到90
				publish(client,bambu::msg::runGcode(std::string("M190 S") + std::to_string(bed_target_temper_max)));//恢复原来的热床温度
			}
			//可能还需要恢复原来的热头温度?,不过也可以先读然后存好
			if (extruder.exchange(bed_target_temper) != bed_target_temper) {
				fpr("唤醒换料程序");
				pause_lock = true;
				extruder.notify_one();//唤醒耗材切换
			}
			else if (!pause_lock.load()) {//可能会收到旧消息
				fpr("同一耗材,无需换料");
				publish(client,bambu::msg::print_resume);//无须换料
			}
			if (bed_target_temper_max > 0)
				bed_target_temper = bed_target_temper_max;//必要,恢复温度后,MQTT的更新可能不及时

		}
		else {
			// publish(client,bambu::msg::get_status);//从第二次暂停开始,PAUSE就不会出现在常态消息里,不知道怎么回事
			//还是会的,只是不一定和温度改变在一条json里
		}
	}
	else
		if (bed_target_temper == 0)
			bed_target_temper_max = 0;//打印结束or冷打印版
		else
			bed_target_temper_max = std::max(bed_target_temper,bed_target_temper_max);//不同材料可能底板温度不一样,这里选择维持最高的

	//int print_error_now = doc["print"]["print_error"] | -1;
	//if (print_error_now != -1) {
	//	fpr_value(print_error_now);
	//	if (print_error.exchange(print_error_now) != print_error_now)//@_@这种有变动才唤醒的地方可以合并一下
	//		print_error.notify_one();
	//}

	int ams_status_now = doc["print"]["ams_status"] | -1;
	if (ams_status_now != -1) {
		fpr("asm_status_now:",ams_status_now);
		if (ams_status.exchange(ams_status_now) != ams_status_now)
			ams_status.notify_one();
	}

}//callback



//mqtt事件循环处理(call_back),@_@之后整理一下放在别的地方
void mqtt_event_handler(void* handler_args,esp_event_base_t base,int32_t event_id,void* event_data) {
	auto TAG = "MQTT ";
	fpr("\n事件分发");
	esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);
	esp_mqtt_client_handle_t client = event->client;
	int msg_id = -1;
	switch (esp_mqtt_event_id_t(event_id)) {
	case MQTT_EVENT_CONNECTED:
		fpr(TAG,"MQTT_EVENT_CONNECTED（MQTT连接成功）");
		msg_id = esp_mqtt_client_subscribe(client,config::topic_subscribe.c_str(),1);
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

		callback_fun(client,std::string(event->data));
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


[[nodiscard]] esp_mqtt_client_handle_t mqtt_app_start() {
	//const esp_mqtt_client_config_t mqtt_cfg = {
	//	.broker = {
	//		 .address = {
	//			.uri = config::mqtt_server.c_str()
	//		}//address
	//		,.verification = {
	//			 .use_global_ca_store = false
	//			,.certificate = nullptr
	//			,.certificate_len = 0
	//			,.skip_cert_common_name_check = true
	//		}//verification
	//	}//broker
	//	,.credentials = {
	//		 .username = config::mqtt_username.c_str()
	//		,.authentication = {
	//			.password = config::mqtt_pass.c_str()
	//		}//authentication
	//	}//credentials
	//};//mqtt_cfg

	esp_mqtt_client_config_t mqtt_cfg{};
	mqtt_cfg.broker.address.uri = config::mqtt_server.c_str();
	//mqtt_cfg.broker.verification.use_global_ca_store = false;
	mqtt_cfg.broker.verification.skip_cert_common_name_check = true;
	mqtt_cfg.credentials.username = config::mqtt_username.c_str();
	mqtt_cfg.credentials.authentication.password = config::mqtt_pass.c_str();

	fpr("[APP] Free memory: ",esp_get_free_heap_size(),"bytes");
	esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
	/* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
	esp_mqtt_client_register_event(client,MQTT_EVENT_ANY,mqtt_event_handler,nullptr);
	esp_mqtt_client_start(client);
	return client;
}



void work(esp_mqtt_client_handle_t client) {//需要更好名字
	int old_extruder = extruder;
	while (true) {

		esp::gpio_out(esp::LED_R,true);
		fpr("等待换料");
		extruder.wait(old_extruder);
		esp::gpio_out(esp::LED_R,false);

		publish(client,bambu::msg::uload);
		fpr("发送了退料命令,等待退料完成");
		mstd::atomic_wait_un(ams_status,退料完成需要退线);
		fpr("退料完成,需要退线,等待退线完");

		esp::gpio_out(config::motors[old_extruder - 1].backward,true);
		mstd::delay(config::uload_time);
		//这里可以检查一下线确实退出来了
		esp::gpio_out(config::motors[old_extruder - 1].backward,false);
		mstd::atomic_wait_un(ams_status,退料完成);//应该需要这个wait,打印机或者网络偶尔会卡

		fpr("进线");
		old_extruder = extruder;
		esp::gpio_out(config::motors[old_extruder - 1].forward,true);

		// {
		// 	publish(client,bambu::msg::load);
		// 	fpr("发送了料进线命令,等待进线完成");
		// 	mstd::atomic_wait_un(ams_status,262);
		// 	mstd::delay(2s);
		// 	publish(client,bambu::msg::click_done);
		// 	mstd::delay(2s);
		// 	mstd::atomic_wait_un(ams_status,263);
		// 	publish(client,bambu::msg::click_done);
		// 	mstd::atomic_wait_un(ams_status,进料完成);
		// 	mstd::delay(2s);
		// }

		publish(client,bambu::msg::print_resume);//暂停恢复
		mstd::delay(config::load_time);
		esp::gpio_out(config::motors[old_extruder - 1].forward,false);

		pause_lock = false;
	}//while
}//work
/*
* 似乎外挂托盘的数据也能通过mqtt改动
*/



extern "C" void app_main(void) {


	std::jthread gpio_in_thread([]() {
		using namespace config;
		esp::gpio_set_in(forward_click);//注册需要的中断服务
		esp::gpio_set_in(back_click);

		if (forward_click == GPIO_NUM_NC && back_click == GPIO_NUM_NC)
			return;

		bool forward_state = false;
		bool back_state = false;

		while (true) {
			esp::gpin_t gpin_v{ GPIO_NUM_NC,std::chrono::steady_clock::now() };
			if (xQueueReceive(esp::gpio_channle,&gpin_v,portMAX_DELAY)) {
				const gpio_num_t& io_num = std::get<0>(gpin_v);

				if (io_num == forward_click) {

					mstd::delay(50ms);
					if (forward_state != !gpio_get_level(io_num)) {//状态改变
						forward_state = !forward_state;
						fpr("前向微动触发,变为",forward_state);
						int now_extruder = extruder;//假定前向微动触发肯定是非换料时间
						esp::gpio_out(config::motors[now_extruder - 1].forward,forward_state);

					}
				}
				else if (io_num == back_click) {
					mstd::delay(50ms);
					if (back_state != !gpio_get_level(io_num)) {//状态改变
						back_state = !forward_state;
						fpr("后向微动触发,变为",back_state);
						int now_extruder = extruder;//后向微动触发可能是换料时间,要在work确保好extruder的改变时机
						esp::gpio_out(config::motors[now_extruder - 1].backward,back_state);
					}
				}
				else {
					fpr("未知中断,非预期行为");
				}
			}
		}//while
		}//[](){}
	);//jthread




	/*esp_err_t ret = */nvs_flash_init();//初始化nvs存储空间
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());


	esp::wifi_init_sta(config::WIFI_SSID,config::WIFI_PASS);



	fpr("开始MQTT");
	auto client = mqtt_app_start();
	mstd::delay(10s);

	// publish(client,bambu::msg::runGcode("M190 S10"));
	// publish(client,bambu::msg::led_on);

	work(client);



	int cnt = 0;
	while (true) {
		mstd::delay(2000ms);
		esp::gpio_out(esp::LED_R,cnt % 2);
		++cnt;
	}


	// return;
}


//标准库IO库之前哪里看到会暴涨二进制,之后改一下
//还需要整理一下,规范一点
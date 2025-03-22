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


#include <sys/param.h>

#if __has_include("config.hpp")
#include "config.hpp"
#else
#include "config_example.hpp"
#endif

#include "include/tools.hpp"
#include "include/espIO.hpp"
#include "include/espWIFI.hpp"
#include "include/espMQTT.hpp"
#include "include/bambu.hpp"
#include <ArduinoJson.hpp>





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
	// mstd::delay(2s);
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
	static int nozzle_target_temper = -1;
	bed_target_temper = doc["print"]["bed_target_temper"] | bed_target_temper;
	// nozzle_target_temper = doc["print"]["nozzle_target_temper"] | nozzle_target_temper;
	std::string gcode_state = doc["print"]["gcode_state"] | "unkonw";

	// fpr("nozzle_target_temper:",nozzle_target_temper);

	// return;
	if (bed_target_temper > 0 && bed_target_temper < 17) {//读到的温度是通道
		if (gcode_state == "PAUSE") {
			// mstd::delay(4s);//确保暂停动作(3.5s)完成
			mstd::delay(4500ms);//貌似4s还是有可能会哟ubug
			if (bed_target_temper_max > 0) {//似乎热床置零会导致热端固定到90
				publish(client,bambu::msg::runGcode(
					std::string("M190 S") + std::to_string(bed_target_temper_max)//恢复原来的热床温度
					// + std::string(R"(\nM109 S255)")//提前升温,9系命令自带阻塞,应该无法使两条一起生效
				));
			}

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
		mstd::delay(config::load_time);
		esp::gpio_out(config::motors[old_extruder - 1].forward,false);

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
		mstd::delay(4s);//等待命令落实
		esp::gpio_out(config::motors[old_extruder - 1].forward,true);
		mstd::delay(7s);//辅助进料时间,@_@也可以考虑放在config
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
	auto client = esp::mqtt_app_start<callback_fun>(config::mqtt_server,config::mqtt_username,config::mqtt_pass);
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
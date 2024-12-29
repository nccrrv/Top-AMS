#pragma once

#include "include/tools.h"
#include <string>
#include <array>
// #include "driver/gpio.h"
#include "include/espIO.h"

namespace config {

	using std::string;

	inline const string WIFI_SSID = "wifi_ssid";
	inline const string WIFI_PASS = "wifi_pass";


	inline const string ip = "192.168.1.1";
	inline const string mqtt_server = "mqtts://" + ip + ":8883";
	inline const string mqtt_username = "bblp";
	inline const string mqtt_pass = "00000000";

	inline const string device_serial = "XXXXXXXXXXXXXXXX";

	inline const string topic_subscribe = "device/" + device_serial + "/report";
	inline const string topic_publish = "device/" + device_serial + "/request";


	inline const auto load_time = 15s;//进料运转时间
	inline const auto uload_time = 15s;//退料运转时间


	struct motor {
		gpio_num_t forward = GPIO_NUM_NC;
		gpio_num_t backward = GPIO_NUM_NC;

		//微动预留@_@
		gpio_num_t stop_forward = GPIO_NUM_NC;
		motor(gpio_num_t f,gpio_num_t b,gpio_num_t sf) :forward(f),backward(b),stop_forward(sf) {}
		//@_@

		motor() = default;
		motor(gpio_num_t f,gpio_num_t b) :forward(f),backward(b) {}
	};//motor

	inline std::array<motor,16> motors{
		 motor{GPIO_NUM_3,GPIO_NUM_2}//通道1,前向GPIO,后向GPIO
		,motor{GPIO_NUM_3,GPIO_NUM_2,GPIO_NUM_4}//通道1,前向GPIO,后向GPIO,停止前向GPIO
		// ,motor{GPIO_NUM_5,GPIO_NUM_4}//通道2...
		,motor{GPIO_NUM_NC,GPIO_NUM_NC}//通道3
		,motor{GPIO_NUM_NC,GPIO_NUM_NC}//通道4
	};//motors


	inline auto __gpio_out_false = +[](void* arg) {
		gpio_num_t IO = *static_cast<gpio_num_t*>(arg);

		esp::gpio_out(IO,false);
		};

	inline mstd::call_once __motors_Init(
		[](std::array<motor,16>& motors) {
			for (size_t i = 0; i < motors.size(); i++) {
				if (motors[i].forward != GPIO_NUM_NC) {//有定义
					esp::gpio_set_in(motors[i].stop_forward,__gpio_out_false,&motors[i].forward);//注册需要的中断服务
				}
			}
		},motors
	);


}//config
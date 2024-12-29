#pragma once

#include "include/tools.h"
#include <string>
#include <array>
#include "driver/gpio.h"

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

		motor() = default;
		motor(gpio_num_t f,gpio_num_t b) :forward(f),backward(b) {}
	};//motor

	inline std::array<motor,16> motors{
		 motor{GPIO_NUM_3,GPIO_NUM_2}//通道1,前向GPIO,后向GPIO
		,motor{GPIO_NUM_3,GPIO_NUM_2}//通道1,前向GPIO,后向GPIO
		// ,motor{GPIO_NUM_5,GPIO_NUM_4}//通道2...
		,motor{GPIO_NUM_NC,GPIO_NUM_NC}//通道3
		,motor{GPIO_NUM_NC,GPIO_NUM_NC}//通道4
	};
}//config
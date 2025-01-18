#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"


#include "esp_system.h"
#include "esp_partition.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"



#include "driver/gpio.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "tools.hpp"
// #include <array>
#include <vector>
#include <chrono>
#include <tuple>

namespace esp {

	inline constexpr gpio_num_t LED_R = GPIO_NUM_12;
	inline constexpr gpio_num_t LED_L = GPIO_NUM_13;


	inline std::vector<gpio_config_t> gpio_state;
	inline mstd::call_once __gpio_state_init{ [](std::vector<gpio_config_t>& v) {
		for (size_t i = 0; i < 64; i++)
			v.emplace_back(
				1ull << i,				//设置要操作的接口,掩码结构
				GPIO_MODE_DISABLE,		// 设置是输入还是输出
				GPIO_PULLUP_DISABLE,	//开关上拉
				GPIO_PULLDOWN_DISABLE,	//开关下拉
				GPIO_INTR_DISABLE		// 开关中断
				);
	},gpio_state };


	//普通输出
	inline void gpio_out(gpio_num_t IO,bool value) {

		if (gpio_state[IO].mode != GPIO_MODE_OUTPUT) {
			gpio_config_t io_conf = {
				1ull << IO,			// 设置要操作的接口,掩码结构
				GPIO_MODE_OUTPUT,	// 设置是输入还是输出
				GPIO_PULLUP_DISABLE,	//开关上拉
				GPIO_PULLDOWN_DISABLE,	//开关下拉
				GPIO_INTR_DISABLE		// 开关中断
			};
			gpio_config(&io_conf);
		}

		gpio_set_level(IO,value);
		// fpr(IO,' ',value);
	}//gpio_out

	//开漏输出
	inline void gpio_out_OD(gpio_num_t IO,bool value) {
		if (gpio_state[IO].mode != GPIO_MODE_OUTPUT_OD) {
			gpio_config_t io_conf = {
				1ull << IO,			 // 设置要操作的接口,掩码结构
				GPIO_MODE_OUTPUT_OD, // 设置是输入还是输出
				GPIO_PULLUP_DISABLE,	//开关上拉
				GPIO_PULLDOWN_DISABLE,	//开关下拉
				GPIO_INTR_DISABLE		// 开关中断
			};
			gpio_config(&io_conf);
		}

		gpio_set_level(IO,value);
	}//gpio_out_OD



	using gpin_t = std::tuple<gpio_num_t,std::chrono::steady_clock::time_point>;//可能叫button更好

	inline QueueHandle_t gpio_channle = xQueueCreate(1,sizeof(gpin_t));//中断队列

	//统一将IO脚编号加入队列
	inline void IRAM_ATTR __gpio_isr_handler(void* arg) {
		gpio_num_t gpio_num = (gpio_num_t)(uintptr_t)(arg);
		gpin_t in{ gpio_num,std::chrono::steady_clock::now() };//@_@可能不需要
		xQueueSendFromISR(gpio_channle,&in,NULL);
	}

	inline void gpio_set_in(gpio_num_t IO) {//未来如果有别的需求可以抽象一下,只改中断触发方式
		// static_assert(IO != 9, "BOOT");//?上电前不能下拉，ESP32会进入下载模式
		// static_assert(IO != 11, "11");//GPIO11默认为SPI flash的VDD引脚，需要配置后才能作为GPIO使用
		if (IO == gpio_num_t::GPIO_NUM_NC) return;


		uint64_t pin_mask = 1ull << IO;
		gpio_config_t io_conf = {
			pin_mask,
			GPIO_MODE_INPUT,
			GPIO_PULLUP_ENABLE,//开上拉
			GPIO_PULLDOWN_DISABLE,
			// GPIO_INTR_NEGEDGE//下降沿触发
			// GPIO_INTR_LOW_LEVEL//低电平触发
			GPIO_INTR_ANYEDGE//上下边沿触发
		};
		gpio_config(&io_conf);
		gpio_set_intr_type(IO,GPIO_INTR_ANYEDGE);


		gpio_isr_handler_add(IO,__gpio_isr_handler,(void*)IO);
	}//gpio_set_in

	inline mstd::call_once __gpio_set_init([] {
		gpio_install_isr_service(0);
		});






} // esp
#pragma once

#include "driver/gpio.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "tools.hpp"
// #include <array>
#include <vector>

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
	}//gpio_out

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



	inline QueueHandle_t gpio_channle = xQueueCreate(10,sizeof(gpio_num_t));

	//统一将IO脚编号加入队列
	inline void IRAM_ATTR __gpio_isr_handler(void* arg) {
		gpio_num_t gpio_num = (gpio_num_t)(uintptr_t)(arg);
		xQueueSendFromISR(gpio_channle,&gpio_num,NULL);
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
			GPIO_INTR_LOW_LEVEL//低电平触发
		};
		gpio_config(&io_conf);
		gpio_set_intr_type(IO,GPIO_INTR_ANYEDGE);


		gpio_isr_handler_add(IO,__gpio_isr_handler,(void*)IO);
	}//gpio_set_in

	inline mstd::call_once __gpio_set_init([] {
		gpio_install_isr_service(0);
		});






} // esp
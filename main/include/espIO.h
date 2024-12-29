#pragma once

#include "driver/gpio.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "tools.h"
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


	inline mstd::call_once __gpio_set_init([] {
		gpio_install_isr_service(0);
		});

	template<typename F,typename V>
	void gpio_set_in(gpio_num_t IO,const F& f,V&& v) {//未来如果有别的需求可以抽象一下,只改中断触发方式
		// static_assert(IO != 9, "BOOT");//?上电前不能下拉，ESP32会进入下载模式
		// static_assert(IO != 11, "11");//GPIO11默认为SPI flash的VDD引脚，需要配置后才能作为GPIO使用
		//mstd::call_once __init_once([]() {
		uint64_t pin_mask = 1ull << IO;
		gpio_config_t io_conf = {
			pin_mask,
			GPIO_MODE_INPUT,
			GPIO_PULLUP_ENABLE,//开上拉
			GPIO_PULLDOWN_DISABLE,
			GPIO_INTR_NEGEDGE//开中断
		};

		gpio_config(&io_conf);

		uint32_t io = IO;
		gpio_isr_handler_add(IO,f,(void*)v);
	}//gpio_set_in





} // esp
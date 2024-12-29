#pragma once

#include "driver/gpio.h"
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
	}





} // esp
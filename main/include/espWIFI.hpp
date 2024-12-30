#pragma once
#include "esp_wifi.h"
#include "tools.hpp"
#include <cstring>

namespace esp {

	inline constexpr int esp_maximum_retry = 5;

	inline static EventGroupHandle_t s_wifi_event_group;

	inline constexpr int WIFI_CONNECTED_BIT = 1;
	inline constexpr int WIFI_FAIL_BIT = 2;

	inline void event_handler(void* arg,esp_event_base_t event_base,int32_t event_id,void* event_data) {
		auto TAG = "WIFI ";
		static int s_retry_num = 0;
		if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
			esp_wifi_connect();
		}
		else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
			if (s_retry_num < esp_maximum_retry) {
				esp_wifi_connect();
				s_retry_num++;
				fpr(TAG,"retry to connect to the AP");
			}
			else {
				xEventGroupSetBits(s_wifi_event_group,WIFI_FAIL_BIT);
			}
			fpr(TAG,"connect to the AP fail");
		}
		else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
			ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
			fpr(TAG,"got ip:",IP2STR(&event->ip_info.ip));
			s_retry_num = 0;
			xEventGroupSetBits(s_wifi_event_group,WIFI_CONNECTED_BIT);
		}
	}

	inline void wifi_init_sta(const std::string& ssid,const std::string& pass) {
		auto TAG = "WIFI ";
		s_wifi_event_group = xEventGroupCreate();

		esp_netif_create_default_wifi_sta();

		wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
		ESP_ERROR_CHECK(esp_wifi_init(&cfg));

		esp_event_handler_instance_t instance_any_id;
		esp_event_handler_instance_t instance_got_ip;
		ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
			ESP_EVENT_ANY_ID,
			&esp::event_handler,
			NULL,
			&instance_any_id));
		ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
			IP_EVENT_STA_GOT_IP,
			&event_handler,
			NULL,
			&instance_got_ip));

		wifi_config_t wifi_config{};
		std::memcpy(wifi_config.sta.ssid,ssid.c_str(),ssid.size());
		std::memcpy(wifi_config.sta.password,pass.c_str(),pass.size());


		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
		ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA,&wifi_config));
		ESP_ERROR_CHECK(esp_wifi_start());

		fpr(TAG,"wifi_init_sta finished.");

		/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
		 * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
		EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
			WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
			pdFALSE,
			pdFALSE,
			portMAX_DELAY);

		/* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
		 * happened. */
		if (bits & WIFI_CONNECTED_BIT) {
			fpr(TAG,"connected to ap:",ssid);
		}
		else if (bits & WIFI_FAIL_BIT) {
			fpr(TAG,"Failed to connect to SSID:",ssid," password:",pass);
		}
		else {
			fpr(TAG,"UNEXPECTED EVENT");
		}
	}//wifi_init_sta

	//直接从官方例子拷的,稍微改了一下
}//esp
/*
* Copyright (C) 2025-2026 by nccrrv
* SPDX-License-Identifier: AGPL-3.0-or-later
* 
* smartWIFI.hpp
* Example:
* 
*   smartWIFI wifi;
*   smartWIFI wifi("SSID", "PASS");//也可以先写死
*   wifi.connected();//阻塞,等待配网,有过SSID后直接开始连接
*   wifi.reset();//重置WiFi配置
*/

#pragma once
#include "nvs_value.hpp"
#include <Arduino.h>
#include <WiFi.h>

#include "espIO.hpp"

namespace mesp {

    struct smartWIFI {

        nvs_value<Exstring<32>> Wifi_ssid{"Wifi_ssid", ""};
        nvs_value<Exstring<64>> Wifi_pass{"Wifi_pass", ""};

        smartWIFI() = default;

        template <typename T, typename Y>
        smartWIFI(T&& ssid, Y&& pass) {
            Wifi_ssid = std::forward<T>(ssid);
            Wifi_pass = std::forward<Y>(pass);
        }

        //连接,阻塞
        void connected() {
            size_t cnt = 0;

            if (Wifi_ssid == "") {
                WiFi.mode(WIFI_AP_STA);
                WiFi.beginSmartConfig();

                while (!WiFi.smartConfigDone()) {
                    gpio_out(config::WIFI_LED, cnt % 2);//慢闪,等待配网
                    ++cnt;
                    fpr("Waiting for SmartConfig");
                    mstd::delay(1000ms);
                }

                Wifi_ssid = WiFi.SSID().c_str();
                Wifi_pass = WiFi.psk().c_str();

            } else {
                WiFi.begin(Wifi_ssid.get().c_str(), Wifi_pass.get().c_str());
            }

            // 等待WiFi连接到路由器
            while (WiFi.status() != WL_CONNECTED) {
                gpio_out(config::WIFI_LED, cnt % 2);//快闪,配网中
                ++cnt;
                fpr("Waiting for WiFi Connected");
                mstd::delay(250ms);
            }

            gpio_out(config::WIFI_LED, false);//关闭,连接成功
            fpr("WiFi Connected to AP");
            fpr("IP Address: ", (int)WiFi.localIP()[0], ".", (int)WiFi.localIP()[1], ".", (int)WiFi.localIP()[2], ".", (int)WiFi.localIP()[3]);
        }

        void reset() {
            Wifi_ssid = "";
            Wifi_pass = "";
        }

    };//smartWIFI
}//mesp
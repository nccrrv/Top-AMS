#pragma once
#include "mesp/wsValue.hpp"


namespace topams {

    using mesp::ws_nvs_value;
    using mesp::wsValue;
    using mstd::Exstring;




    struct motor {
        int32_t id = 0;
        gpio_num_t forward = GPIO_NUM_NC;
        gpio_num_t backward = GPIO_NUM_NC;

        ws_nvs_value<Exstring<24>> name{"ext" + Exstring<3>(id) + "_name", "PETG"};//耗材名
        ws_nvs_value<Exstring<8>> color{"ext" + Exstring<3>(id) + "_color", "#ffffff"};//颜色
        ws_nvs_value<int32_t> next_channel{"ext" + Exstring<3>(id) + "_next", id};//续料通道

        ws_nvs_value<int32_t> temper{"ext" + Exstring<3>(id) + "_temper", 250};//续料温度
        ws_nvs_value<int32_t> load_time{"ext" + Exstring<3>(id) + "_load", 6000};
        ws_nvs_value<int32_t> uload_time{"ext" + Exstring<3>(id) + "_uload", 5000};

        motor() = default;
        motor(int32_t i, gpio_num_t f = GPIO_NUM_NC, gpio_num_t b = GPIO_NUM_NC)
            : id(i),
              forward(f),
              backward(b),
              name("ext" + Exstring<3>(id) + "_name", "PETG"),
              color("ext" + Exstring<3>(id) + "_color", "#ffffff"),
              next_channel("ext" + Exstring<3>(id) + "_next", id),
              temper("ext" + Exstring<3>(id) + "_temper", 250),
              load_time("ext" + Exstring<3>(id) + "_load", 6000),
              uload_time("ext" + Exstring<3>(id) + "_uload", 5000) {}
    };// motor

    inline std::array<motor, 16>
        motors{{
            {1, GPIO_NUM_2, GPIO_NUM_3},//1
            {2, GPIO_NUM_10, GPIO_NUM_6},//2
            {3, GPIO_NUM_5, GPIO_NUM_4},//3
            {4, GPIO_NUM_8, GPIO_NUM_9},//4
            {5, GPIO_NUM_0, GPIO_NUM_1},//5
            {6, GPIO_NUM_20, GPIO_NUM_21},//6
            {7, GPIO_NUM_12, GPIO_NUM_13},//7
            {8, GPIO_NUM_18, GPIO_NUM_19},//8
            {9, GPIO_NUM_NC, GPIO_NUM_NC},//9
            {10, GPIO_NUM_NC, GPIO_NUM_NC},//10
            {11, GPIO_NUM_NC, GPIO_NUM_NC},//11
            {12, GPIO_NUM_NC, GPIO_NUM_NC},//12
            {13, GPIO_NUM_NC, GPIO_NUM_NC},//13
            {14, GPIO_NUM_NC, GPIO_NUM_NC},//14
            {15, GPIO_NUM_NC, GPIO_NUM_NC},//15
            {16, GPIO_NUM_NC, GPIO_NUM_NC}//16
        }};
    //16通道的情况下使用地址

    //进料微动
    inline gpio_num_t forward_click = GPIO_NUM_7;

    struct mqtt_config {
        inline static mesp::ws_nvs_value<Exstring<16>> bambu_ip{"bambu_ip", ""};
        inline static ws_nvs_value<Exstring<16>> device_serial{"device_serial", ""};
        inline static ws_nvs_value<Exstring<9>> MQTT_pass{"MQTT_pass", ""};

        static auto server_ip() {
            return "mqtts://" + bambu_ip.get_value() + ":8883";
        }
        static auto client() {
            return Exstring<5>("bblp");
        }
        static auto passward() {
            return MQTT_pass.get_value();
        }
        static auto topic_subscribe() {
            return "device/" + device_serial.get_value() + "/report";
        }
        static auto topic_publish() {
            return "device/" + device_serial.get_value() + "/request";
        }
    };//mqtt_config
}
#pragma once

#include "espIO.hpp"
#include "esptools.hpp"
// #include <WString.h>
#include <array>


#include "web_sync.hpp"

namespace config {

    // using string = String;
    using std::string;

    struct motor {
        gpio_num_t forward = GPIO_NUM_NC;
        gpio_num_t backward = GPIO_NUM_NC;

        motor() = default;
        motor(gpio_num_t f, gpio_num_t b) : forward(f), backward(b) {}
        //通道管理需要的东西都加到这里和web同步
    };// motor


    //**********************用户配置区开始******************************
    //**********************用户配置区开始******************************
    //**********************用户配置区开始******************************


    inline mesp::wsStoreValue<int> load_time("load_time", 6000);// 进料运转时间
    inline mesp::wsStoreValue<int> uload_time("uload_time", 5000);// 退料运转时间

    inline const auto forward_click = GPIO_NUM_7;// 进料微动,缓存机构预留
    // inline const auto back_click = GPIO_NUM_NC;   // 退料微动

    inline gpio_num_t LED_R = GPIO_NUM_12;
    inline gpio_num_t LED_L = GPIO_NUM_13;//暂未使用


inline std::array<motor,16> motors{//电机要使用的GPIO
		 motor{GPIO_NUM_1,GPIO_NUM_0}//通道1,前向GPIO,后向GPIO
		,motor{GPIO_NUM_19,GPIO_NUM_18}//通道2
		,motor{GPIO_NUM_3,GPIO_NUM_2}//通道3
		,motor{GPIO_NUM_6,GPIO_NUM_10}//通道4 
        ,motor{GPIO_NUM_12,GPIO_NUM_7}
		,motor{GPIO_NUM_8,GPIO_NUM_5}

   /* inline std::array<motor, 16> motors{
        // 电机要使用的GPIO
        motor{GPIO_NUM_2, GPIO_NUM_3},// 通道1,前向GPIO,后向GPIO
        motor{GPIO_NUM_10, GPIO_NUM_6},// 通道2
        motor{GPIO_NUM_5, GPIO_NUM_4},// 通道3
        motor{GPIO_NUM_8, GPIO_NUM_9},// 通道4
        motor{GPIO_NUM_0, GPIO_NUM_1},// 通道5
        motor{GPIO_NUM_20, GPIO_NUM_21},// 通道6
        motor{GPIO_NUM_12, GPIO_NUM_13},// 通道7,GPIO12,13为灯,避免冲突需要将灯定义改为NC
        motor{GPIO_NUM_18, GPIO_NUM_19},// 通道8,GPIO18,19和USB冲突,需要带串口芯片或者无协议供电
*/

    };// motors
    // 小白用户自定义电机使用GPIO时
    // 请仔细查询你开发板的针脚定义
    // 像合众esp32C3,应该避开usb使用的18,19,LED灯的12,13
    // GPIO_NUM_NC表示不使用




    mesp::wsValue<bool> MQTT_done("MQTT_done", false);
    mesp::wsStoreValue<string> bambu_ip("bambu_ip", "192.168.1.1");
    mesp::wsStoreValue<string> MQTT_pass("MQTT_pass", "");
    mesp::wsStoreValue<string> device_serial("device_serial", "");

    //**********************用户配置区结束******************************
    //**********************用户配置区结束******************************
    //**********************用户配置区结束******************************


    inline string mqtt_server(const string& ip = bambu_ip) { return "mqtts://" + ip + ":8883"; }
    inline string mqtt_username = "bblp";
    inline string topic_subscribe(const string& _device_serial = device_serial) {
        return "device/" + _device_serial + "/report";
    }
    inline string topic_publish(const string& _device_serial = device_serial) {
        return "device/" + _device_serial + "/request";
    }

    



}// config
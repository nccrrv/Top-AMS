#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

// #undef F

#include "bambu.hpp"
#include "esptools.hpp"
#include "mesp/channel.hpp"
#include "mesp/espIO.hpp"
#include "mesp/mqtt.hpp"
#include "mesp/smartWIFI.hpp"
#include "mesp/wsValue.hpp"
#include "topams_config.hpp"

namespace mstd {
    template <typename T>
    void atomic_wait_un(std::atomic<T>& value, T target) {//@_@可以考虑加入mstd
        auto old_value = value.load();
        while (old_value != target) {
            value.wait(old_value);
            old_value = value;
        }
    }
}//mstd

#include "index.hpp"

using mesp::Mqttclient;
using mesp::webfpr;
using mesp::webfpr_lager;
using mstd::Exstring;
using mstd::fpr;

//重启按钮动作
inline mstd::call_once restart_command(
    []() {
        mesp::command_emplace<+[]() {
            ESP.restart();
        }>("restart");
    });



inline void state_event(const Exstring<128>& msg) {}

using async_work_type = std::function<void()>;
inline mesp::channel<async_work_type, 10> async_channel;

namespace topams {

    inline mesp::Mqttclient mqttclient;


#ifdef MOTOR_16

#else
    //8通道
    template <typename... T>
    inline void motor_run(int32_t motor_id, bool fwd, const T&... t) {
        if (motor_id < 1 || motor_id > topams::motors.size()) {
            webfpr("电机编号错误:" + Exstring(motor_id));
            return;
        }
        motor_id--;


        const auto& motor_forward = topams::motors[motor_id].forward;
        const auto& motor_backward = topams::motors[motor_id].backward;
        const auto& load_time = topams::motors[motor_id].load_time.get_value();
        const auto& uload_time = topams::motors[motor_id].uload_time.get_value();


        if (motor_forward == mesp::config::LED_R) [[unlikely]] {
            mesp::config::LED_R = GPIO_NUM_NC;
            mesp::config::LED_L = GPIO_NUM_NC;//这里假定了是一对使用,然后先前进了!_!
        }//使用到了通道7,关闭代码中的LED控制

        if (fwd) {
            mesp::gpio_out(motor_forward, true);
            if constexpr (sizeof...(T) == 0)
                mstd::delay_ms(load_time);
            else
                mstd::delay(t...);// 使用传入的延时
            mesp::gpio_out(motor_forward, false);
        } else {
            mesp::gpio_out(motor_backward, true);
            if constexpr (sizeof...(T) == 0)
                mstd::delay_ms(uload_time);
            else
                mstd::delay(t...);// 使用传入的延时
            mesp::gpio_out(motor_backward, false);
        }

    }//motor_run

#endif


    inline volatile int32_t hw_switch = 0;//@_@应该是atomic
    inline mesp::ws_nvs_value<int32_t> extruder("extruder", 0);// 1-16, 0表示无耗材
    inline std::atomic<bool> pause_lock{false};// 暂停锁
    std::atomic<int32_t> ams_status = -1;
    std::atomic<int32_t> nozzle_target_temper = -1;



    //@brief 换料
    void change_filament(const Mqttclient& client, int32_t old_extruder, int32_t new_extruder) {
        webfpr("开始换料");
        // esp::gpio_out(config::LED_R, false);
        if (topams::motors[new_extruder - 1].load_time.get_value() > 0) {//使用固定时间进料@_@
            webfpr("使用固定时间进料");
            // ws_extruder = std::to_string(old_extruder) + string(" → ") + std::to_string(new_extruder);

            // publish(client, bambu::msg::uload);
            client.publish(bambu::msg::runGcode(
                "M109 S" + Exstring(topams::motors[old_extruder - 1].temper.get_value()) + "\nM620 S255\nT255\nM621 S255\n"));//新的快速退料!_!
            webfpr("发送了退料命令,等待退料完成");
            mstd::atomic_wait_un(ams_status, bambu::status::退料完成需要退线);
            webfpr("退料完成,需要退线,等待退线完");

            motor_run(old_extruder, false);// 退线

            mstd::atomic_wait_un(ams_status, bambu::status::退料完成);// 应该需要这个wait,打印机或者网络偶尔会卡

            int32_t new_nozzle_temper = topams::motors[new_extruder - 1].temper.get_value();
            client.publish(bambu::msg::runGcode("M109 S" + Exstring(new_nozzle_temper)));
            while (nozzle_target_temper.load() < new_nozzle_temper - 5) {
                mstd::delay(500ms);// 等待热端温度达到目标温度
            }
            // mstd::delay(5s);//先5s,时间可能取决于热端到250的速度,一个想法是把拉高热端提前能省点时间,但是比较难控制
            //@_@也可以读热端温度,不过如果读==250的话,肯定是挤出机先转,或者可以考虑条件为>240之类

            webfpr("进线");
            client.publish(bambu::msg::runGcode("G1 E150 F500"));//旋转热端齿轮辅助进料
            mstd::delay(3s);//还是需要延迟,命令落实没这么快
            motor_run(new_extruder, true);// 进线

            // extruder = new_extruder;//换料完成
            extruder.set_value(new_extruder);//换料完成


            // {//旧的使用进线程序的进料过程
            // 	publish(client,bambu::msg::load);
            // 	fpr("发送了料进线命令,等待进线完成");
            // 	mstd::atomic_wait_un(ams_status,262);
            // 	mstd::delay(2s);
            // 	publish(client,bambu::msg::click_done);
            // 	mstd::delay(2s);
            // 	mstd::atomic_wait_un(ams_status,263);
            // 	publish(client,bambu::msg::click_done);
            // 	mstd::atomic_wait_un(ams_status,进料完成);
            // 	mstd::delay(2s);
            // }

            client.publish(bambu::msg::print_resume);// 暂停恢复
        } else {//自动判定进料时间
            webfpr("小绿点判定进料");
            webfpr("还没写");
        }
    }


    //上料
    void load_filament(int32_t new_extruder) {
        if (!mqttclient.is_connected()) {
            webfpr("MQTT未连接,无法上料");
            return;
        }

        if (new_extruder < 1 || new_extruder > topams::motors.size()) {
            webfpr("不支持的上料通道");
            return;
        }

        webfpr("开始进料");

        {//新写的N20上料
            mqttclient.publish(bambu::msg::get_status);//查询小绿点
            mstd::delay(3s);//等待查询结果
            if (hw_switch == 1) {//有料需要退料
                int32_t old_extruder = extruder;
                if (old_extruder == 0) {
                    webfpr("请设置当前所使用通道,否则无法退料再进料");
                    return;
                }
                if (old_extruder == new_extruder) {
                    webfpr("当前通道已经是" + Exstring(new_extruder) + "无需上料");
                    return;
                }

                // publish(__client, bambu::msg::uload);
                mqttclient.publish(bambu::msg::runGcode(
                    "M109 S" + Exstring(topams::motors[old_extruder - 1].temper.get_value()) + "\nM620 S255\nT255\nM621 S255\n"));//新的快速退料
                webfpr("发送了退料命令,等待退料完成");
                mstd::atomic_wait_un(ams_status, bambu::status::退料完成需要退线);
                webfpr("退料完成,需要退线,等待退线完");

                motor_run(old_extruder, false);// 退线

                mstd::atomic_wait_un(ams_status, bambu::status::退料完成);// 应该需要这个wait,打印机或者网络偶尔会卡
                webfpr("退线完成");
            }//if (hw_switch == 1)


            {//进料
                int32_t new_nozzle_temper = topams::motors[new_extruder - 1].temper.get_value();
                mqttclient.publish(bambu::msg::runGcode("M109 S" + Exstring(new_nozzle_temper)));
                while (nozzle_target_temper.load() < new_nozzle_temper - 5) {
                    mstd::delay(500ms);// 等待热端温度达到目标温度
                }

                webfpr("进线");
                int32_t try_num = 0;
                do {
                    mqttclient.publish(bambu::msg::runGcode("G1 E150 F500"));//旋转热端齿轮辅助进料
                    mstd::delay(3s);//还是需要延迟,命令落实没这么快
                    motor_run(new_extruder, true);// 进线
                    mqttclient.publish(bambu::msg::get_status);//再次查询小绿点
                    mstd::delay(3s);//等待查询结果
                    if (hw_switch != 1)
                        webfpr("没检测到小绿点,尝试再次进料");
                    if (++try_num == 5) {
                        webfpr("尝试多次仍未检测到小绿点,进料失败,请重新检查参数");
                        return;
                    }
                } while (hw_switch != 1);

                //有小绿点,进料成功,!_!,注意,还有一种可能,就是刚好到了小绿点的位置,但是没被工具头齿轮咬入


                extruder = new_extruder;//换料完成


                mqttclient.publish(
                    bambu::msg::runGcode(
                        Exstring("G1 E100 F180\n")//简单冲刷100
                        + Exstring("M400\n") + Exstring("M106 P1 S255\n")//风扇全速
                        + Exstring("M400 S3\n")//冷却
                        + Exstring("G1 X -3.5 F18000\nG1 X -13.5 F3000\nG1 X -3.5 F18000\nG1 X -13.5 F3000\nG1 X -3.5 F18000\nG1 X -13.5 F3000\n")//切屎
                        + Exstring("M400\nM106 P1 S0\nM109 S90\n")));//结束并降温到90
            }
            webfpr("上料完成");
        }//N20上料

        return;

    }//load_filament


    //mqtt事件处理
    inline void data_event(const mesp::Mqttclient& client, const mesp::Mqttclient::data_cache_type& data) {

        {//json回传
            static mesp::ws_nvs_value<bool> printer_json{"printer_json", false};
            if (printer_json.get_value())
                webfpr_lager(data);
            Exstring start_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            // webfpr(start_time);
        }//json回传


        static StaticJsonDocument<mesp::Mqttclient::data_cache_type::max_size() + 1024> doc;
        doc.clear();
        DeserializationError error = deserializeJson(doc, data.c_str());

        static int32_t bed_target_temper = -1;
        static int32_t bed_target_temper_max = 0;
        // static int32_t nozzle_target_temper = -1;

        bed_target_temper = doc["print"]["bed_target_temper"] | bed_target_temper;
        nozzle_target_temper.store(doc["print"]["nozzle_target_temper"] | nozzle_target_temper.load());
        mstd::Exstring<32> gcode_state = doc["print"]["gcode_state"] | "unkonw";
        hw_switch = doc["print"]["hw_switch_state"] | hw_switch;

        {

            if (bed_target_temper > 0 && bed_target_temper < 17) {// 读到的温度是通道

                if (gcode_state == "PAUSE") {
                    // mstd::delay(4s);//确保暂停动作(3.5s)完成
                    // mstd::delay(4500ms);// 貌似4s还是有可能会有bug,貌似bug本质是以前发gcode忘了\n,现在应该不用延时
                    if (bed_target_temper_max > 0) {// 似乎热床置零会导致热端固定到90
                        client.publish(
                            bambu::msg::runGcode("M190 S" + mstd::Exstring(bed_target_temper_max)));// 恢复原来的热床温度
                    }

                    int32_t old_extruder = extruder.get_value();
                    int32_t new_extruder = bed_target_temper;
                    if (old_extruder != new_extruder) {//旧通道不等于新通道
                        mstd::fpr("唤醒换料程序");
                        pause_lock = true;
                        async_channel.emplace([&client, old_extruder, new_extruder]() {
                            change_filament(client, old_extruder, new_extruder);
                        });
                    } else if (!pause_lock.load()) {// 可能会收到旧消息
                        mstd::fpr("同一耗材,无需换料");
                        client.publish(bambu::msg::runGcode("M190 S" + mstd::Exstring(bed_target_temper_max)));//恢复原来的热床温度
                        mstd::delay(1000ms);//确保暂停动作完成
                        client.publish(bambu::msg::print_resume);// 无须换料
                    }
                    if (bed_target_temper_max > 0)
                        bed_target_temper = bed_target_temper_max;// 必要,恢复温度后,MQTT的更新可能不及时

                } else {
                    // publish(client,bambu::msg::get_status);//从第二次暂停开始,PAUSE就不会出现在常态消息里,不知道怎么回事
                    // 还是会的,只是不一定和温度改变在一条json里
                }
            } else if (bed_target_temper == 0)
                bed_target_temper_max = 0;// 打印结束
            else
                bed_target_temper_max = std::max(bed_target_temper, bed_target_temper_max);// 不同材料可能底板温度不一样,这里选择维持最高的
        }

        int ams_status_now = doc["print"]["ams_status"] | -1;
        if (ams_status_now != -1) {
            fpr("asm_status_now:", ams_status_now);
            if (ams_status.exchange(ams_status_now) != ams_status_now)
                ams_status.notify_one();
        }

    }//date_event




    //注册command命令
    inline mstd::call_once register_command(
        []() {
            mesp::command_emplace<+[](int32_t motor_id) {
                motor_run(motor_id, true);
            }>("motor_forward");

            mesp::command_emplace<+[](int32_t motor_id) {
                motor_run(motor_id, false);
            }>("motor_backward");

            mesp::command_emplace<+[](int32_t extruder_id) {
                load_filament(extruder_id);
            }>("load_filament");

            mesp::command_emplace<+[]() {
                mqttclient.topic_publish = mqtt_config::topic_publish();
                mqttclient.topic_subscribe = mqtt_config::topic_subscribe();
                mqttclient.connect(
                    mqtt_config::server_ip(),
                    mqtt_config::client(),
                    mqtt_config::passward());
            }>("MQTT_connect");
        });





    //微动缓冲程序,要优化
    void Task1(void* param) {
        mesp::gpio_set_in(topams::forward_click);

        while (true) {
            int32_t level = gpio_get_level(topams::forward_click);

            if (level == 0) {
                int32_t now_extruder = extruder.get_value();
                webfpr("微动触发");
                motor_run(now_extruder, true, 1s);// 进线
            }

            mstd::delay(50ms);
        }
    }//微动缓冲程序

    TaskHandle_t Task1_handle;//微动任务


}//topams




extern "C" void app_main(void) {
    using namespace mesp;

    ws_command_init();

    fpr("main函数开始");
    fpr("wsValue数量:", wsValue_state.map.size());


    //异步任务处理,mini线程池
    std::thread async_thread([]() {
        while (true) {
            auto task = async_channel.pop();
            task();
        }
    });
    xTaskCreate(topams::Task1, "Task1", 2048, NULL, 1, &topams::Task1_handle);//微动任务


    smartWIFI wifi;
    // smartWIFI wifi("SSID", "PASS");//也可以先写死
    wifi.connected();

    AsyncWebServer server(80);
    AsyncWebSocket& ws = mesp::ws_server;


    topams::mqttclient.data_event = topams::data_event;


    mesp::ws_nvs_value<bool> boot_connect{"boot_connect", false};//开机连接
    if (boot_connect.get_value()) {
        topams::mqttclient.topic_publish = topams::mqtt_config::topic_publish();
        topams::mqttclient.topic_subscribe = topams::mqtt_config::topic_subscribe();
        topams::mqttclient.connect(
            topams::mqtt_config::server_ip(),
            topams::mqtt_config::client(),
            topams::mqtt_config::passward());
    }


    {//服务器配置部分
        server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
            // request->send(200, "text/html", web.c_str());
            request->send(200, "text/html", web.data());
        });

        server.addHandler(&ws);

        // 设置未找到路径的处理
        server.onNotFound([](AsyncWebServerRequest* request) {
            request->send(404, "text/plain", "404: Not found");
        });

        // 启动服务器
        server.begin();
        fpr("HTTP 服务器已启动");
    }


    fpr_mem();

    //loop
    size_t count = 0;
    for (;;) {
        ws.cleanupClients();//清理断开的ws客户端
        mstd::delay(2s);
        if (count % 100 == 0)
            fpr_mem();
        ++count;
    }
    return;
}

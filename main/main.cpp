#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

#include "esptools.hpp"

#include "bambu.hpp"
#include "espIO.hpp"
#include "espMQTT.hpp"

#include "channel.hpp"

#include "esp_timer.h"

#include "web_sync.hpp"

#if __has_include("localconfig.hpp")
#include "localconfig.hpp"//发布时去掉这个
#endif


using std::string;

//新添加的变量
TaskHandle_t Task1_handle;//微动任务
volatile int hw_switch = 0;
TaskHandle_t Task2_handle;//延时调试信息


//分割

int bed_target_temper_max = 0;
mesp::wsStoreValue<int> extruder("extruder", 0);// 1-16, 0表示无耗材

int sequence_id = -1;
std::atomic<int> print_error = 0;//打印错误代码用于判断自动续料 50364437是A1 50364420是P1
std::atomic<int> ams_status = -1;
std::atomic<bool> pause_lock{false};// 暂停锁
std::atomic<int> nozzle_target_temper = -1;
//std::atomic<int> hw_switch{0};//小绿点, 其实是布尔

inline constexpr int 正常 = 0;
inline constexpr int 退料完成需要退线 = 260;//A1
//inline constexpr int 退料完成需要退线 = 259;//P1
inline constexpr int 退料完成 = 0;// 同正常
inline constexpr int 进料检查 = 262;
inline constexpr int 进料冲刷 = 263;// 推测
inline constexpr int 进料完成 = 768;
//@_@应该放在一个枚举类里

AsyncWebServer server(80);
// AsyncWebSocket ws("/ws");
AsyncWebSocket& ws = mesp::ws_server;//先直接用全局的ws_server

inline mstd::channel_lock<std::function<void()>> async_channel;//异步任务通道

inline string last_ws_log = "日志初始化";//可能会有多个webfpr,非线程安全注意,现在单核先不管


//@brief WebSocket消息打印
inline void webfpr(AsyncWebSocket& ws, const string& str) {
    mstd::fpr("wsmsg: ", str);
    JsonDocument doc;
    doc["log"] = str;
    String msg;
    serializeJson(doc, msg);
    ws.textAll(msg);
    last_ws_log = str;
}

//@brief WebSocket消息打印
inline void webfpr(const string& str) {
    mstd::fpr("wsmsg: ", str);
    JsonDocument doc;
    doc["log"] = str;
    String msg;
    serializeJson(doc, msg);
    ws.textAll(msg);
    last_ws_log = str;
}

// @brief 控制电机运行(前向或后向)
// @param moter_id 电机编号,从 1 开始
// @param fwd 标识方向，true 表示前向，false 表示后向
// @param t 延时
template <typename T>
inline void motor_run(int motor_id, bool fwd, T&& t) {
    if (motor_id < 1 || motor_id > config::motors.size()) {
        webfpr(std::string("电机编号错误:") + std::to_string(motor_id));
        return;
    }
    motor_id--;
    if (config::motors[motor_id].forward == config::LED_R) [[unlikely]] {
        config::LED_R = GPIO_NUM_NC;
        config::LED_L = GPIO_NUM_NC;
    }//使用到了通道7,关闭代码中的LED控制
    if (fwd) {
        esp::gpio_out(config::motors[motor_id].forward, true);
        mstd::delay(std::forward<T>(t));// 使用传入的延时
        esp::gpio_out(config::motors[motor_id].forward, false);
    } else {
        esp::gpio_out(config::motors[motor_id].backward, true);
        mstd::delay(std::forward<T>(t));// 使用传入的延时
        esp::gpio_out(config::motors[motor_id].backward, false);
    }
}//motor_run


// @brief 控制电机运行(前向或后向)
// @param moter_id 电机编号,从 1 开始
// @param fwd 标识方向，true 表示前向，false 表示后向
inline void motor_run(int motor_id, bool fwd) {
    motor_run(motor_id, fwd,
              fwd ? config::motors[motor_id - 1].load_time.get_value() : config::motors[motor_id - 1].uload_time.get_value());
}//motor_run





//@brief 发布消息到MQTT服务器
void publish(esp_mqtt_client_handle_t client, const std::string& msg) {
    esp::gpio_out(config::LED_L, true);
    // mstd::delay(2s);
    fpr("发送消息:", msg);
    int msg_id = esp_mqtt_client_publish(client, config::topic_publish().c_str(), msg.c_str(), msg.size(), 0, 0);
    //@_@这里的publish用到了topic_publish(默认),耦合了
    if (msg_id < 0)
        fpr("发送失败");
    else
        fpr("发送成功,消息id=", msg_id);
    // fpr(TAG, "binary sent with msg_id=%d", msg_id);
    esp::gpio_out(config::LED_L, false);
    // mstd::delay(2s);//@_@这些延时还可以调
    //我觉得延时还是加在程序里好调试
}




//换料
void change_filament(esp_mqtt_client_handle_t client, int old_extruder, int new_extruder) {

    constexpr auto fpr = [](const string& r) { webfpr(ws, r); };//重设一下fpr

    fpr("开始换料");
    // esp::gpio_out(config::LED_R, false);
    if (config::motors[new_extruder - 1].load_time > 0) {//使用固定时间进料@_@
        fpr("使用固定时间进料");
        // ws_extruder = std::to_string(old_extruder) + string(" → ") + std::to_string(new_extruder);
        //ws_extruder不再使用,可以考虑给前端加一个状态表示正在换料@_@


        // publish(client, bambu::msg::uload);
        publish(client, bambu::msg::runGcode(
                            "M109 S" + std::to_string(config::motors[old_extruder - 1].temper.get_value()) + "\nM620 S255\nT255\nM621 S255\n"));//新的快速退料
        fpr("发送了退料命令,等待退料完成");
        mstd::atomic_wait_un(ams_status, 退料完成需要退线);
        fpr("退料完成,需要退线,等待退线完");

        motor_run(old_extruder, false);// 退线

        mstd::atomic_wait_un(ams_status, 退料完成);// 应该需要这个wait,打印机或者网络偶尔会卡

        int new_nozzle_temper = config::motors[new_extruder - 1].temper.get_value();
        publish(client, bambu::msg::runGcode("M109 S" + std::to_string(new_nozzle_temper)));
        while (nozzle_target_temper.load() < new_nozzle_temper - 5) {
            mstd::delay(500ms);// 等待热端温度达到目标温度
        }
        // mstd::delay(5s);//先5s,时间可能取决于热端到250的速度,一个想法是把拉高热端提前能省点时间,但是比较难控制
        //@_@也可以读热端温度,不过如果读==250的话,肯定是挤出机先转,或者可以考虑条件为>240之类

        fpr("进线");
        publish(client, bambu::msg::runGcode("G1 E150 F500"));//旋转热端齿轮辅助进料
        mstd::delay(3s);//还是需要延迟,命令落实没这么快
        motor_run(new_extruder, true);// 进线

        extruder = new_extruder;//换料完成

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

        publish(client, bambu::msg::print_resume);// 暂停恢复
    } else {//自动判定进料时间
        fpr("小绿点判定进料");
        fpr("还没写");
    }
}// work
/*
 * 似乎外挂托盘的数据也能通过mqtt改动
 */


esp_mqtt_client_handle_t __client;

//上料
void load_filament(int new_extruder) {
    // __client;//先用这个,之后解耦出来,应该穿参进来,改好clien的生存期和错误回报就行

    if (!(new_extruder > 0 && new_extruder <= config::motors.size())) {
        webfpr("不支持的上料通道");
        return;
    }

    webfpr("开始进料");

    {//新写的N20上料
        publish(__client, bambu::msg::get_status);//查询小绿点
        mstd::delay(3s);//等待查询结果
        if (hw_switch == 1) {//有料需要退料
            int old_extruder = extruder;
            if (old_extruder == 0) {
                webfpr("请设置当前所使用通道,否则无法退料再进料");
                return;
            }
            if (old_extruder == new_extruder) {
                webfpr("当前通道已经是" + std::to_string(new_extruder) + "无需上料");
                return;
            }
            // ws_extruder = std::to_string(old_extruder) + string(" → ") + std::to_string(new_extruder);
            // publish(__client, bambu::msg::uload);
            publish(__client, bambu::msg::runGcode(
                                  "M109 S" + std::to_string(config::motors[old_extruder - 1].temper.get_value()) + "\nM620 S255\nT255\nM621 S255\n"));//新的快速退料
            webfpr("发送了退料命令,等待退料完成");
            mstd::atomic_wait_un(ams_status, 退料完成需要退线);
            webfpr("退料完成,需要退线,等待退线完");

            motor_run(old_extruder, false);// 退线

            mstd::atomic_wait_un(ams_status, 退料完成);// 应该需要这个wait,打印机或者网络偶尔会卡
            webfpr("退线完成");
        }//if (hw_switch == 1)
        {//进料
            int new_nozzle_temper = config::motors[new_extruder - 1].temper.get_value();
            publish(__client, bambu::msg::runGcode("M109 S" + std::to_string(new_nozzle_temper)));
            while (nozzle_target_temper.load() < new_nozzle_temper - 5) {
                mstd::delay(500ms);// 等待热端温度达到目标温度
            }
            // mstd::delay(5s);//先5s,时间可能取决于热端到250的速度,一个想法是把拉高热端提前能省点时间,但是比较难控制
            //@_@也可以读热端温度,不过如果读==250的话,肯定是挤出机先转,或者可以考虑条件为>240之类

            webfpr("进线");
            publish(__client, bambu::msg::runGcode("G1 E150 F500"));//旋转热端齿轮辅助进料
            mstd::delay(3s);//还是需要延迟,命令落实没这么快
            motor_run(new_extruder, true);// 进线

            /*
            此处应该查下小绿点,如果小绿点没触发的话,G1命令无效
            */

            extruder = new_extruder;//换料完成
            // ws_extruder = std::to_string(new_extruder);// 更新前端显示的耗材编号
            // publish(__client, 一段冲刷gcode,冲刷完后记得降温);@_@
        }
        webfpr("上料完成");
    }//新写的N20上料

    return;

}//load_filament




void work(mesp::Mqttclient& Mqtt) {//之后应该修改好mesp::Mqttclient生命周期@_@
    __client = Mqtt.client;
    Mqtt.subscribe(config::topic_subscribe());// 订阅消息

    int cnt = 0;
    while (true) {
        mstd::delay(20000ms);
        // esp::gpio_out(esp::LED_R, cnt % 2);
        ++cnt;
    }
}

//@brief MQTT回调函数
void callback_fun(esp_mqtt_client_handle_t client, const std::string& json) {// 接受到信息的回调
    // fpr(json);
    using namespace ArduinoJson;
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);

    // mesp::print_memory_info();

    static int bed_target_temper = -1;
    // static int nozzle_target_temper = -1;
    bed_target_temper = doc["print"]["bed_target_temper"] | bed_target_temper;
    nozzle_target_temper.store(doc["print"]["nozzle_target_temper"] | nozzle_target_temper.load());
    std::string gcode_state = doc["print"]["gcode_state"] | "unkonw";
    hw_switch = doc["print"]["hw_switch_state"] | hw_switch;
    // print_error.store(doc["print"]["print_error"] | print_error.load());


    // fpr("hw_switch:" + std::to_string(hw_switch));//小绿点状态
    //int nextChannel1 = config::motors[1].next_channel.get_value();
    //fpr("电机[1]的下一个通道值: " + std::to_string(nextChannel1));
    // fpr("print_error:" + std::to_string(print_error));//打印错误代码




    //@_@这边也有些混乱,实质都是因为打印机网络这边不是很稳定所遗留的写法,需要更好的处理
    if (bed_target_temper > 0 && bed_target_temper < 17) {// 读到的温度是通道
        if (gcode_state == "PAUSE") {
            // mstd::delay(4s);//确保暂停动作(3.5s)完成
            // mstd::delay(4500ms);// 貌似4s还是有可能会有bug,貌似bug本质是以前发gcode忘了\n,现在应该不用延时
            if (bed_target_temper_max > 0) {// 似乎热床置零会导致热端固定到90
                publish(client, bambu::msg::runGcode(
                                    std::string("M190 S") + std::to_string(bed_target_temper_max)// 恢复原来的热床温度
                                    // + std::string(R"(\nM109 S255)")//提前升温,9系命令自带阻塞,应该无法使两条一起生效
                                    ));
            }

            int old_extruder = extruder;
            int new_extruder = bed_target_temper;
            if (old_extruder != new_extruder) {//旧通道不等于新通道
                fpr("唤醒换料程序");
                pause_lock = true;
                async_channel.emplace([=]() {
                    change_filament(client, old_extruder, new_extruder);
                });
            } else if (!pause_lock.load()) {// 可能会收到旧消息
                fpr("同一耗材,无需换料");
                publish(client, bambu::msg::runGcode(std::string("M190 S") + std::to_string(bed_target_temper_max)));//恢复原来的热床温度
                mstd::delay(1000ms);//确保暂停动作完成
                publish(client, bambu::msg::print_resume);// 无须换料
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

    // int print_error_now = doc["print"]["print_error"] | -1;
    // if (print_error_now != -1) {
    //	fpr_value(print_error_now);
    //	if (print_error.exchange(print_error_now) != print_error_now)//@_@这种有变动才唤醒的地方可以合并一下
    //		print_error.notify_one();
    // }

    int ams_status_now = doc["print"]["ams_status"] | -1;
    if (ams_status_now != -1) {
        fpr("asm_status_now:", ams_status_now);
        if (ams_status.exchange(ams_status_now) != ams_status_now)
            ams_status.notify_one();
    }

}// callback

void Task1(void* param) {
    esp::gpio_set_in(config::forward_click);

    while (true) {
        int level = gpio_get_level(config::forward_click);

        if (level == 0) {
            int now_extruder = extruder;
            webfpr("微动触发");
            motor_run(now_extruder, true, 1s);// 进线
        }

        vTaskDelay(20 / portTICK_PERIOD_MS);//延时1000ms=1
    }
}//微动缓冲程序


//延时检测
void Task2(void* param) {
    while (true) {
        mstd::delay(1000ms);
        mesp::time_out++;
        if (mesp::time_out > 8) {
            webfpr("打印机连接超时请检查网络状态");
            mstd::delay(5000ms);
        }
    }//延时打印内存信息
}//其实这个应该放在mqtt那边,作为错误处理的一部分@_@

#include "index.hpp"

volatile bool running_flag{false};

extern "C" void app_main() {
#ifndef LOCAL_CONFIG
    for (size_t i = 0; i < config::motors.size(); i++) {
        auto& x = config::motors[i];
        esp::gpio_out(x.forward, false);
        esp::gpio_out(x.backward, false);
    }//初始化电机GPIO
#endif

    xTaskCreate(Task1, "Task1", 2048, NULL, 1, &Task1_handle);//微动任务
    xTaskCreate(Task2, "Task2", 2048, NULL, 1, &Task2_handle);//延时调试信息


    {// wifi连接部分
        mesp::ConfigStore wificonfig("wificonfig");

        string Wifi_ssid = wificonfig.get("Wifi_ssid", "");
        string Wifi_pass = wificonfig.get("Wifi_pass", "");

        if (Wifi_ssid == "") {
            WiFi.mode(WIFI_AP_STA);
            WiFi.beginSmartConfig();

            int cnt = 0;
            while (!WiFi.smartConfigDone()) {
                delay(1000);
                esp::gpio_out(config::LED_R, cnt % 2);
                ++cnt;
                fpr("Waiting for SmartConfig");
            }

            Wifi_ssid = WiFi.SSID().c_str();
            Wifi_pass = WiFi.psk().c_str();

            wificonfig.set("Wifi_ssid", Wifi_ssid);
            wificonfig.set("Wifi_pass", Wifi_pass);
        } else {
            WiFi.begin(Wifi_ssid.c_str(), Wifi_pass.c_str());
        }

        // 等待WiFi连接到路由器
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            fpr("Waiting for WiFi Connected");
        }

        fpr("WiFi Connected to AP");
        fpr("IP Address: ", (int)WiFi.localIP()[0], ".", (int)WiFi.localIP()[1], ".", (int)WiFi.localIP()[2], ".", (int)WiFi.localIP()[3]);
        esp::gpio_out(config::LED_R, false);
    }// wifi连接部分


    using namespace config;
    using namespace ArduinoJson;
    using std::string;

    //异步任务处理,线程池
    std::thread async_thread([]() {
        while (true) {
            auto task = async_channel.pop();
            task();
        }
    });

    std::binary_semaphore mqtt_Signal{0};

    {//服务器配置部分
        server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
            // request->send(200, "text/html", web.c_str());
            request->send(200, "text/html", web.data());
        });

        // 配置 WebSocket 事件处理
        ws.onEvent([&mqtt_Signal](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
            if (type == WS_EVT_CONNECT) {
                fpr("WebSocket 客户端", client->id(), "已连接\n");
                fpr(last_ws_log);
                webfpr(last_ws_log);

                JsonDocument doc;
                JsonObject root = doc.to<JsonObject>();
                root.createNestedArray("data");// 创建data数组

                for (auto& [name, to_json] : mesp::ws_value_to_json)
                    to_json(doc);// 添加当前值到data数组
                mesp::sendJson(doc);// 发送所有注册的值

                fpr(doc);
            } else if (type == WS_EVT_DISCONNECT) {
                fpr("WebSocket 客户端 ", client->id(), "已断开\n");
            } else if (type == WS_EVT_DATA) {// 处理接收到的数据
                fpr("收到ws数据");
                data[len] = 0;// 确保字符串终止

                JsonDocument doc;
                deserializeJson(doc, data);
                fpr("ws收到的json\n", doc, "\n");


                if (doc.containsKey("data") && doc["data"].is<JsonArray>()) {
                    for (JsonObject obj : doc["data"].as<JsonArray>()) {
                        if (obj.containsKey("name")) {
                            std::string name = obj["name"].as<std::string>();
                            auto it = mesp::ws_value_update.find(name);
                            if (it != mesp::ws_value_update.end()) {
                                it->second(obj);//更新值
                            }

                            if (name == "device_serial") {//需要连接mqtt,放这里感觉有些耦合
                                mqtt_Signal.release();
                            }
                        }
                    }
                }//wsvalue更新部分


                const std::string command = doc["action"]["command"] | string("_null");
                if (command != "_null") {//处理命令json
                    if (command == "motor_forward") {//电机前向控制
                        int motor_id = doc["action"]["value"] | -1;
                        async_channel.emplace(
                            [motor_id]() {
                                motor_run(motor_id, true);
                            });
                    } else if (command == "motor_backward") {//电机前向控制
                        int motor_id = doc["action"]["value"] | -1;
                        async_channel.emplace(
                            [motor_id]() {
                                motor_run(motor_id, false);
                            });
                    } else if (command == "load_filament") {
                        int new_extruder = doc["action"]["value"] | -1;
                        async_channel.emplace(
                            [new_extruder]() {
                                fpr("上料");
                                load_filament(new_extruder);
                            });

                    } else {
                        fpr("未知命令:", command);
                    }
                }//if command

            }//WS_EVT_DATA
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


    {// 打印机Mqtt配置
        if (MQTT_pass != "") {// 有旧数据,可以先连MQTT
            fpr("当前MQTT配置\n", bambu_ip, '\n', MQTT_pass, '\n', device_serial);
            mesp::Mqttclient Mqtt(mqtt_server(bambu_ip), mqtt_username, MQTT_pass, callback_fun);
            webfpr(ws, "MQTT连接中...");
            Mqtt.wait();
            if (Mqtt.connected()) {
                auto temp = last_ws_log;
                webfpr("MQTT连接成功");
                last_ws_log = temp;//@_@或许应该改改webfpr,让它不覆盖
                MQTT_done = true;
                work(Mqtt);
            } else {
                MQTT_done = false;
                //Mqtt错误反馈分类
                webfpr(ws, "MQTT连接错误");
            }
        }//if (MQTT_pass != "")

        while (!MQTT_done) {
            fpr("等待Mqtt配置");
            mqtt_Signal.acquire();// 等待mqtt配置
            fpr("当前MQTT配置\n", bambu_ip, '\n', MQTT_pass, '\n', device_serial);
            mesp::Mqttclient Mqtt(mqtt_server(bambu_ip), mqtt_username, MQTT_pass, callback_fun);
            webfpr(ws, "MQTT连接中...");
            Mqtt.wait();
            if (Mqtt.connected()) {
                webfpr("MQTT连接成功");
                MQTT_done = true;
                work(Mqtt);
            } else {
                //Mqtt错误反馈分类
                MQTT_done = false;

                webfpr(ws, "MQTT连接错误");
            }
        }
    }



    int cnt = 0;
    while (true) {
        mstd::delay(20000ms);
        // esp::gpio_out(esp::LED_R, cnt % 2);
        ++cnt;
    }
    return;
}
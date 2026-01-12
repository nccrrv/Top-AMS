#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <chrono>

#include "esptools.hpp"

#include "bambu.hpp"
#include "espIO.hpp"
#include "espMQTT.hpp"

#include "channel.hpp"

#include "esp_timer.h"

#include "web_sync.hpp"

#if __has_include("localconfig.hpp")
// #include "localconfig.hpp"//发布时去掉这个
#endif


using std::string;

//新添加的变量
TaskHandle_t Task1_handle;//微动任务
volatile int hw_switch = 0;
TaskHandle_t Task2_handle;//延时调试信息


//分割

int bed_target_temper_max = 0;
mesp::wsStoreValue<int> extruder("extruder", 0);// 1-16, 0表示无耗材，默认未设置

int sequence_id = -1;
std::atomic<int> print_error = 0;//打印错误代码用于判断自动续料 50364437是A1 50364420是P1
std::atomic<int> ams_status = -1;
std::atomic<bool> pause_lock{false};// 暂停锁
std::atomic<int> nozzle_target_temper = -1;
//std::atomic<int> hw_switch{0};//小绿点, 其实是布尔

// 系统状态变量，用于前端显示和按钮控制
mesp::wsValue<int> motor_running("motor_running", 0);// 当前运行的电机通道，0表示无电机运行
mesp::wsValue<string> operation_status("operation_status", "idle");// 操作状态: "idle", "changing", "loading"
mesp::wsValue<bool> system_locked("system_locked", false);// 系统锁定状态

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

// RAII状态管理类，确保在异常或提前返回时也能清理状态
struct SystemStateGuard {
    bool active;
    SystemStateGuard() : active(true) {
        system_locked = true;
    }
    ~SystemStateGuard() {
        if (active) {
            system_locked = false;
            operation_status = "idle";
            pause_lock = false;
        }
    }
    void release() {
        active = false;
        system_locked = false;
        operation_status = "idle";
        pause_lock = false;
    }
};


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
    
    // 更新电机运行状态
    motor_running = motor_id + 1; // 恢复为1-based索引
    webfpr(std::string("电机 ") + std::to_string(motor_id + 1) + (fwd ? " 正转" : " 反转"));
    
    if (fwd) {
        esp::gpio_out(config::motors[motor_id].forward, true);
        mstd::delay(std::forward<T>(t));// 使用传入的延时
        esp::gpio_out(config::motors[motor_id].forward, false);
    } else {
        esp::gpio_out(config::motors[motor_id].backward, true);
        mstd::delay(std::forward<T>(t));// 使用传入的延时
        esp::gpio_out(config::motors[motor_id].backward, false);
    }
    
    // 电机运行完成，清除状态
    motor_running = 0;
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
    // 使用RAII确保状态总是被清理
    SystemStateGuard state_guard;
    operation_status = "changing";
    webfpr("开始换料");
    
    // 查询当前通道使用状态（通过hw_switch_state判断是否有料）
    publish(client, bambu::msg::get_status);
    mstd::delay(3s);//等待查询结果
    
    // 检查当前通道是否在使用中（通过hw_switch_state判断）
    bool has_filament = (hw_switch == 1);
    
    // 如果当前记录的通道就是目标通道，且检测到有料，直接继续，无需换料
    if (old_extruder == new_extruder && has_filament) {
        webfpr("当前通道" + std::to_string(new_extruder) + "正在使用中，无需换料");
        extruder = new_extruder;// 确保记录正确
        state_guard.release(); // 提前释放状态
        publish(client, bambu::msg::print_resume);// 暂停恢复
        return;
    }
    
    // 需要换料的情况：退出旧通道
    // 如果old_extruder == 0，说明当前无耗材，直接进料新通道
    if (old_extruder > 0 && old_extruder <= config::motors.size()) {
        if (!has_filament) {
            // 检测到无料，只需要退出当前通道，不需要退料流程
            webfpr("检测到无料，当前通道" + std::to_string(old_extruder) + "直接退线");
            motor_run(old_extruder, false);// 退线
        } else {
            // 检测到有料，需要完整的退料流程
            webfpr("检测到有料，当前通道" + std::to_string(old_extruder) + "执行退料");
            if (config::motors[old_extruder - 1].load_time > 0) {
                publish(client, bambu::msg::runGcode(
                                    "M109 S" + std::to_string(config::motors[old_extruder - 1].temper.get_value()) + "\nM620 S255\nT255\nM621 S255\n"));//新的快速退料
                webfpr("发送了退料命令,等待退料完成");
                if (!mstd::atomic_wait_un_timeout(ams_status, 退料完成需要退线, 120s)) {
                    webfpr("等待退料完成超时，可能打印机未响应");
                    return; // RAII会自动清理状态
                }
                webfpr("退料完成,需要退线,等待退线完");

                motor_run(old_extruder, false);// 退线

                if (!mstd::atomic_wait_un_timeout(ams_status, 退料完成, 30s)) {
                    webfpr("等待退料完成状态超时，继续执行");
                    // 继续执行，不返回，因为退线已完成
                }
            }
        }
    } else if (old_extruder == 0) {
        webfpr("当前无耗材，直接进料新通道");
    }
    
    // 进料新通道
    if (config::motors[new_extruder - 1].load_time > 0) {//使用固定时间进料@_@
        webfpr("使用固定时间进料到通道" + std::to_string(new_extruder));
        // ws_extruder = std::to_string(old_extruder) + string(" → ") + std::to_string(new_extruder);
        //ws_extruder不再使用,可以考虑给前端加一个状态表示正在换料@_@

        int new_nozzle_temper = config::motors[new_extruder - 1].temper.get_value();
        publish(client, bambu::msg::runGcode("M109 S" + std::to_string(new_nozzle_temper)));
        auto temp_deadline = std::chrono::steady_clock::now() + 300s; // 5分钟超时
        while (nozzle_target_temper.load() < new_nozzle_temper - 5) {
            if (std::chrono::steady_clock::now() > temp_deadline) {
                webfpr("等待热端温度超时，可能打印机未响应");
                return; // RAII会自动清理状态
            }
            mstd::delay(500ms);// 等待热端温度达到目标温度
        }
        // mstd::delay(5s);//先5s,时间可能取决于热端到250的速度,一个想法是把拉高热端提前能省点时间,但是比较难控制
        //@_@也可以读热端温度,不过如果读==250的话,肯定是挤出机先转,或者可以考虑条件为>240之类

        webfpr("进线");
        publish(client, bambu::msg::runGcode("G1 E150 F500"));//旋转热端齿轮辅助进料
        mstd::delay(3s);//还是需要延迟,命令落实没这么快
        motor_run(new_extruder, true);// 进线

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

        // 换料完成后再更新extruder，避免在换料过程中被callback_fun读取到新值
        extruder = new_extruder;//换料完成
        webfpr("换料完成: 通道" + std::to_string(new_extruder));

        publish(client, bambu::msg::print_resume);// 暂停恢复
    } else {//自动判定进料时间
        webfpr("小绿点判定进料");
        webfpr("功能未实现，无法完成换料");
        state_guard.release(); // 提前释放状态
        return;
    }
    
    // 正常完成，释放状态
    state_guard.release();
}// change_filament
/*
 * 似乎外挂托盘的数据也能通过mqtt改动
 */


esp_mqtt_client_handle_t __client;

//上料
void load_filament(int new_extruder) {
    // __client;//先用这个,之后解耦出来,应该穿参进来,改好clien的生存期和错误回报就行

    // 检查系统是否被锁定（换料或上料进行中）
    if (system_locked.get_value() || pause_lock.load()) {
        webfpr("系统正在执行其他操作，请稍后再试");
        return;
    }

    // 检查__client是否有效
    if (__client == nullptr) {
        webfpr("MQTT客户端未初始化，无法执行上料");
        return;
    }

    if (!(new_extruder > 0 && new_extruder <= config::motors.size())) {
        webfpr("不支持的上料通道");
        return;
    }

    // 使用RAII确保状态总是被清理
    SystemStateGuard state_guard;
    operation_status = "loading";
    webfpr("开始进料");

    {//新写的N20上料
        publish(__client, bambu::msg::get_status);//查询小绿点
        mstd::delay(3s);//等待查询结果
        
        // 进料前检查状态：通过hw_switch_state判断是否有料
        bool has_filament = (hw_switch == 1);
        
        if (has_filament) {//有料需要检查通道
            int old_extruder = extruder.get_value();
            if (old_extruder == 0) {
                webfpr("检测到有料但未设置当前通道，请先设置当前通道");
                state_guard.release(); // 提前释放状态
                return;
            }
            if (old_extruder == new_extruder) {
                // 通道一样，跳过进料
                webfpr("当前通道已经是" + std::to_string(new_extruder) + "，且检测到有料，跳过进料");
                state_guard.release(); // 提前释放状态
                return;
            }
            // 通道变更，需要先退料
            webfpr("检测到有料且通道变更(" + std::to_string(old_extruder) + " → " + std::to_string(new_extruder) + ")，执行退料");
            publish(__client, bambu::msg::runGcode(
                                  "M109 S" + std::to_string(config::motors[old_extruder - 1].temper.get_value()) + "\nM620 S255\nT255\nM621 S255\n"));//新的快速退料
            webfpr("发送了退料命令,等待退料完成");
            if (!mstd::atomic_wait_un_timeout(ams_status, 退料完成需要退线, 120s)) {
                webfpr("等待退料完成超时，可能打印机未响应");
                return; // RAII会自动清理状态
            }
            webfpr("退料完成,需要退线,等待退线完");

            motor_run(old_extruder, false);// 退线

            if (!mstd::atomic_wait_un_timeout(ams_status, 退料完成, 30s)) {
                webfpr("等待退料完成状态超时，继续执行");
                // 继续执行，不返回，因为退线已完成
            }
            webfpr("退线完成");
        } else {
            // 无料，直接进料
            webfpr("检测到无料，直接进料");
        }//if (has_filament)
        {//进料
            int new_nozzle_temper = config::motors[new_extruder - 1].temper.get_value();
            publish(__client, bambu::msg::runGcode("M109 S" + std::to_string(new_nozzle_temper)));
            auto temp_deadline = std::chrono::steady_clock::now() + 300s; // 5分钟超时
            while (nozzle_target_temper.load() < new_nozzle_temper - 5) {
                if (std::chrono::steady_clock::now() > temp_deadline) {
                    webfpr("等待热端温度超时，可能打印机未响应");
                    return; // RAII会自动清理状态
                }
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

            publish(__client,
                    bambu::msg::runGcode(
                        std::string("G1 E100 F180\n")//简单冲刷100
                        + std::string("M400\n") + std::string("M106 P1 S255\n")//风扇全速
                        + std::string("M400 S3\n")//冷却
                        + std::string("G1 X -3.5 F18000\nG1 X -13.5 F3000\nG1 X -3.5 F18000\nG1 X -13.5 F3000\nG1 X -3.5 F18000\nG1 X -13.5 F3000\n")//切屎
                        + std::string("M400\nM106 P1 S0\nM109 S90\n")));//结束并降温到90
        }
        webfpr("上料完成");
    }//新写的N20上料

    // 正常完成，释放状态
    state_guard.release();
    
    return;

}//load_filament




void work(mesp::Mqttclient& Mqtt) {//之后应该修改好mesp::Mqttclient生命周期@_@
    __client = Mqtt.client;
    Mqtt.subscribe(config::topic_subscribe());// 订阅消息

    // 应用启动时检查小绿点状态并初始化通道
    webfpr("检查挤出机状态...");
    publish(__client, bambu::msg::get_status);
    mstd::delay(3s);//等待查询结果
    
    // 根据hw_switch_state判断当前是否有料
    if (hw_switch == 1) {
        // 有料，默认设置为通道1
        if (extruder.get_value() == 0) {
            extruder = 1;
            webfpr("检测到挤出机有料，默认设置为通道1");
        } else {
            webfpr("检测到挤出机有料，当前通道: " + std::to_string(extruder.get_value()));
        }
    } else {
        // 无料，设置为空
        extruder = 0;
        webfpr("检测到挤出机无料，当前通道设置为空");
    }

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
            // 如果正在换料中，忽略新的换料请求，避免重复触发
            if (pause_lock.load() || system_locked.get_value()) {
                fpr("换料进行中，忽略新的换料请求");
                return;
            }
            
            // mstd::delay(4s);//确保暂停动作(3.5s)完成
            // mstd::delay(4500ms);// 貌似4s还是有可能会有bug,貌似bug本质是以前发gcode忘了\n,现在应该不用延时
            
            // 保存通道号，避免在恢复热床温度时丢失
            int new_extruder = bed_target_temper;
            
            // 验证新通道的有效性
            if (new_extruder < 1 || new_extruder > config::motors.size()) {
                fpr("无效的通道编号: " + std::to_string(new_extruder));
                if (bed_target_temper_max > 0) {
                    publish(client, bambu::msg::runGcode(std::string("M190 S") + std::to_string(bed_target_temper_max)));//恢复原来的热床温度
                }
                mstd::delay(1000ms);
                publish(client, bambu::msg::print_resume);
                return;
            }
            
            // 先恢复热床温度（如果bed_target_temper_max > 0）
            if (bed_target_temper_max > 0) {// 似乎热床置零会导致热端固定到90
                publish(client, bambu::msg::runGcode(
                                    std::string("M190 S") + std::to_string(bed_target_temper_max)// 恢复原来的热床温度
                                    // + std::string(R"(\nM109 S255)")//提前升温,9系命令自带阻塞,应该无法使两条一起生效
                                    ));
            }

            int old_extruder = extruder;
            
            if (old_extruder != new_extruder) {//旧通道不等于新通道
                fpr("唤醒换料程序: 通道" + std::to_string(old_extruder) + " → 通道" + std::to_string(new_extruder));
                pause_lock = true;
                // 使用捕获值而不是引用，避免bed_target_temper被修改影响
                async_channel.emplace([=]() {
                    change_filament(client, old_extruder, new_extruder);
                });
            } else {// 同一通道，无需换料
                if (!pause_lock.load()) {// 可能会收到旧消息
                    fpr("同一耗材,无需换料");
                    if (bed_target_temper_max > 0) {
                        publish(client, bambu::msg::runGcode(std::string("M190 S") + std::to_string(bed_target_temper_max)));//恢复原来的热床温度
                    }
                    mstd::delay(1000ms);//确保暂停动作完成
                    publish(client, bambu::msg::print_resume);// 无须换料
                } else {
                    // pause_lock已设置，说明换料正在进行中，忽略此消息
                    fpr("换料进行中，忽略重复消息");
                }
            }
            // 注意：不要在换料过程中恢复bed_target_temper，避免影响换料流程
            // 换料完成后，bed_target_temper会自然恢复为bed_target_temper_max

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
            // 检查辅助进料开关是否开启
            if (config::assist_feeding_enabled.get_value() == 1) {
                int now_extruder = extruder.get_value();
                if (now_extruder > 0 && now_extruder <= config::motors.size()) {
                    webfpr("微动触发，辅助进料");
                    motor_run(now_extruder, true, 1s);// 进线
                } else {
                    webfpr("微动触发，但当前无有效通道");
                }
            } else {
                webfpr("微动触发，但辅助进料已关闭");
            }
        }

        mstd::delay(50ms);
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
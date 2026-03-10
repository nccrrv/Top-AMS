#pragma once
#include "mstd/Exstring.hpp"

namespace bambu {
    using mstd::Exstring;

    namespace msg {

        //运行Gcode
        template <size_t N>
        inline constexpr auto runGcode(const Exstring<N>& code) {
            return R"({"print":{"command":"gcode_line","param" : ")" + code + R"(\n", "sequence_id" : "0"}})";//Gcode结尾一定要\n
        }
        template <size_t N>
        inline constexpr auto runGcode(const char (&code)[N]) {
            return runGcode(Exstring(code));
        }

        //暂停打印
        inline constexpr Exstring print_pause = R"({"print": {"command": "pause","sequence_id" : "0"}})";

        //恢复打印
        inline constexpr Exstring print_resume = R"({"print": {"command": "resume","sequence_id" : "0"}})";

        //进料
        inline constexpr Exstring load = R"({"print":{"command":"ams_change_filament","curr_temp":245,"sequence_id":"10","tar_temp":245,"target":254},"user_id":"1"})";

        //退料
        inline constexpr Exstring uload = R"({"print":{"command":"ams_change_filament","curr_temp":245,"sequence_id":"1","tar_temp":245,"target":255},"user_id":"1"})";
        //@_@如果说是两个_temp是温度,但是实际还是250,可能是没有细分到个位数,也可能是命令理解有误

        //@_@之前的快速退料也要确定一下,需要温度吗?

        inline constexpr Exstring click_done = R"({"print":{"command":"ams_control","param":"done","sequence_id":"1"},"user_id":"1"})";

        inline constexpr Exstring chick_resuem =
            R"({ "print":{"command":"ams_control", "param" : "resume", "sequence_id" : "20030"} })";//推测是进料重试按钮

        inline constexpr Exstring error_clean = R"({"print":{"command": "clean_print_error","sequence_id":"1"},"user_id":"1"})";

        inline constexpr Exstring get_status = R"({"pushing": {"sequence_id": "0", "command": "pushall"}})";


        inline const Exstring led_on =
            R"({"system": {"command": "ledctrl","led_node": "chamber_light","led_mode": "on"}})";
    }//msg


    namespace status {
        inline constexpr int32_t 正常 = 0;
        inline constexpr int32_t 退料完成需要退线 = 260;//A1
        //inline constexpr int32_t 退料完成需要退线 = 259;//P1
        inline constexpr int32_t 退料完成 = 0;// 同正常
        inline constexpr int32_t 进料检查 = 262;
        inline constexpr int32_t 进料冲刷 = 263;// 推测
        inline constexpr int32_t 进料完成 = 768;
    }//status


    /*
    已知A1/A1mini 1.04固件下
    使用Mqtt发送热床调节M140,热端调节M104均无效
    旧版本固件或拓竹自家软件发送有效
    因拓竹网络层闭源,测试需要抓包,较为繁琐
    现阶段调温都用M190,M109
    */

}//bambu

#pragma once
#include <string>

namespace bambu {
    using std::string;

    namespace msg {

        inline string runGcode(const string& code) {
            return string(R"(
{ "print":{"command":"gcode_line","param" : ")" + code + R"(", "sequence_id" : "0"} })");
        }


        //暂停打印
        inline const string print_pause = R"({"print": {"command": "pause","sequence_id" : "0"}})";

        //恢复打印
        inline const string print_resume = R"({"print": {"command": "resume","sequence_id" : "0"}})";

        //进料
        inline const string load = R"({"print":{"command":"ams_change_filament","curr_temp":245,"sequence_id":"10","tar_temp":245,"target":254},"user_id":"1"})";

        //退料
        inline const string uload = R"({"print":{"command":"ams_change_filament","curr_temp":245,"sequence_id":"1","tar_temp":245,"target":255},"user_id":"1"})";
        //@_@如果说是两个_temp是温度,但是实际还是250,可能是没有细分到个位数,也可能是命令理解有误


        inline const string click_done = R"({"print":{"command":"ams_control","param":"done","sequence_id":"1"},"user_id":"1"})";
        inline const string chick_resuem = R"(
		{ "print":{"command":"ams_control", "param" : "resume", "sequence_id" : "20030", "reason" : "success", "result" : "success"} }
)";//推测是进料重试按钮

        inline const string error_clean = R"({"print":{"command": "clean_print_error","sequence_id":"1"},"user_id":"1"})";

        inline const string get_status = R"({"pushing": {"sequence_id": "0", "command": "pushall"}})";


        inline const string led_on = R"({
			"system": {
				"command": "ledctrl",
				"led_node": "chamber_light",
				"led_mode": "on"
			}
		})";
    }//msg

}//bambu

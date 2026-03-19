/*
* Copyright (C) 2025-2026 by nccrrv
* SPDX-License-Identifier: AGPL-3.0-or-later
* 
* wsValue.hpp
* 前后端自动同步的值类型,也能存入flash
*
*  Example:
*   mesp::wsValue<int> ex{"键名", 123};//仅仅前后端同步,赋值就为初始值
*   mesp::ws_nvs_value<int> ex{"key", 123};//在上述的基础加了同步存入flash
*   对应的前后同步json
*   {
*    "data" : [ {
*        "name" : "key", 
*        "value" : 123
*    } ]
*   }
*
*   void 要触发的动作(){};
*   void 要触发的动作_带值处理(int){};
*   注意函数签名要符合,lambda记得加+
*
*   command_emplace<要触发的动作>("command_name");
*   对应的前后同步json
*   {
*       "action":{
*           "command":"command_name",
*           "value":0
*       }
*   }
*/

#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

#undef F

#include "channel.hpp"
#include "nvs_value.hpp"


#include <algorithm>
// #include <flat_map>
#include "../mstd/flat_map.hpp"
#include <vector>



ARDUINOJSON_BEGIN_PUBLIC_NAMESPACE

template <size_t N>
struct Converter<mstd::Exstring<N>> : private detail::VariantAttorney {

    using value_type = mstd::Exstring<N>;
    static void toJson(value_type src, JsonVariant dst) {
        detail::VariantData::setString(getData(dst), detail::adaptString(src.c_str()), getResourceManager(dst));
    }

    static value_type fromJson(JsonVariantConst src) {
        auto data = getData(src);
        const char* p = data ? data->asString().c_str() : "";
        return value_type(p);
    }

    static bool checkJson(JsonVariantConst src) {
        auto data = getData(src);
        return data && data->isString();
    }
};
// 抄的Converter<const char*>,改了下
ARDUINOJSON_END_PUBLIC_NAMESPACE

namespace mesp {

    //全局的ws服务
    inline AsyncWebSocket ws_server("/ws");//现在似乎也没有使用非全局的ws的需求,就先统一为这个了

    struct wsValue_fun {
        std::function<void(const JsonObject&)> set_value;
        std::function<void(JsonDocument&)> get_json;//这里不用function就只能上协程了
    };

    //wsValue的全局注册表
    // inline std::vector<wsValue_fun> wsValue_state;//@_@这里还是堆,注意
    inline mstd::flat_map<key_type, wsValue_fun> wsValue_state;


    //发送json到前端
    inline void sendJson(const JsonDocument& doc, AsyncWebSocket& ws = ws_server) {
        const size_t json_size = measureJson(doc);
        constexpr size_t max_json_size = 4096;//目前是3682
        if (json_size < max_json_size) {
            static mstd::lock_key key;//不池化会爆栈,thread_local用不了
            mstd::RAII_lock l(key);
            static Exstring<max_json_size> msg;
            serializeJson(doc, msg.data(), msg.max_size());
            ws.textAll(msg.c_str());
        } else {
            fpr("json过长 ", json_size);
            String msg;
            serializeJson(doc, msg);
            ws.textAll(msg);
        }
    };//sendJson

}//mesp

namespace Meta {
    template <size_t N>
    struct get_value_type<mstd::Exstring<N>> {
        using type = mstd::Exstring<N>;
    };
    //@_@这里处理的不是很优雅,但是也确实要特化,只是不应该用get_value_type
}// namespace Meta


namespace mesp {
    template <typename T>
    //约束T类型@_@这里
    struct wsValue {
        using value_type = Meta::get_value_type_t<T>;
        using const_value_type = const value_type;

        template <typename Y>
        struct __data_type {
            key_type _key;
            Y _value;

            template <typename... V>
            __data_type(const key_type& k, V&&... v) : _key(k), _value(std::forward<V>(v)...) {}

            auto& value(this auto&& This) noexcept {
                return This._value;
            }
            auto& key(this auto&& This) noexcept {
                return This._key;
            }
        };

        template <typename Y>
        struct __data_type<nvs_value<Y>> {
            nvs_value<Y> data;

            template <typename... V>
            __data_type(const key_type& k, V&&... v) : data(k, std::forward<V>(v)...) {}

            auto& value(this auto&& This) noexcept {
                return This.data;
            }
            auto& key(this auto&& This) noexcept {
                return This.data.key;
            }
        };

        __data_type<T> data;

        template <typename... V>
        wsValue(const key_type& k, V&&... v) : data(k, std::forward<V>(v)...) {
            wsValue_state.emplace(k, wsValue_fun{
                                         [this](const JsonObject& obj) { this->set_value(obj); },
                                         [this](JsonDocument& obj) { this->to_json(obj); }});
        }
        ~wsValue() {
            wsValue_state.erase(data.key());
        }

        wsValue(const wsValue&) = delete;
        wsValue& operator=(const wsValue&) = delete;
        wsValue(wsValue&&) = delete;
        wsValue& operator=(wsValue&&) = delete;

        const_value_type& get_value() const noexcept {
            return data.value();// item["value"]=能直接接受EXstring类型
        }

        operator const_value_type&() const noexcept {
            return get_value();
        }


        void set_value(const value_type& v) {
            data.value() = v;
            update();
        }

        void set_value(const char* v)
            requires mstd::is_Exstring_v<value_type> {
            data.value() = value_type(v);//value实际是nvs_value<Exstring<N>>,char*不能直接转换到nvs_value
            update();
        }//

        template <size_t N>
        void set_value(const char (&v)[N])
            requires mstd::is_Exstring_v<value_type> {
            data.value() = value_type(v);
            update();
        }


        void set_value(const JsonObject& obj) {//非基本类型,就需要特化这个成员函数
            if (!obj.isNull()) {//json为空就只更新前端状态的意思
                set_value(obj["value"].as<value_type>());
            } else
                update();
        }

        wsValue& operator=(const value_type& v) {
            set_value(v);
            return *this;
        }


        //构造对应的json
        void to_json(JsonDocument& doc) const {
            JsonArray jsondata = doc["data"].as<JsonArray>();
            JsonObject item = jsondata.createNestedObject();
            item["name"] = data.key();
            item["value"] = get_value();
        }

        //构造对应的json且发送
        void update() const {
            // StaticJsonDocument<256> doc;//@_@这里还没搞定
            JsonDocument doc;
            JsonObject root = doc.to<JsonObject>();
            root.createNestedArray("data");// 创建data数组
            to_json(doc);// 添加当前值到data数组
            // if (doc.overflowed())
            // fpr("<256>doc overflowed");
            sendJson(doc);
        }


        bool operator==(const value_type& v) const noexcept {
            return data.value() == v;
        }

    };//wsValue<T>


    template <typename T>
    using ws_nvs_value = wsValue<nvs_value<T>>;

    //ws_nvs_value使用和nvs_value一样,多一个自动同步到前端的功能



    //下面的感觉不是很适合放在wsValue,但一时也想不到放哪里@_@



    //command 部分

    using command_name_type = mstd::Exstring<32>;
    using command_fun_type = void (*)(const JsonDocument&);
    inline mstd::flat_map<command_name_type, command_fun_type> command_state;

    template <typename T>
    struct get_Args {};
    template <typename T>
    struct get_Args<void (*)(T)> {
        using type = T;
    };

    inline channel_lock<std::function<void()>> ws_command_channel;

    inline void ws_command_init() {
        static std::thread ws_command_thread{[]() {
            while (true) {
                auto task = mesp::ws_command_channel.pop();
                task();
            }
        }};
    }

    template <auto f>
    inline void command_emplace(const command_name_type& command_name) {
        using command_aync_type = std::function<void()>;

        using F = decltype(f);
        auto ef = [](const JsonDocument& doc) {
            if constexpr (Meta::base_same<void (*)(), F>)
                ws_command_channel.emplace(f);
            else {
                using T = get_Args<F>::type;
                T value = doc["action"]["value"];
                ws_command_channel.emplace(
                    [=]() {
                        f(value);
                    });
            }
        };
        command_state.emplace(command_name, ef);
    }




    //@brief WebSocket消息打印
    inline void webfpr(const Exstring<128>& str) {
        constexpr static size_t max_size = 256;

        mstd::fpr("wsmsg: ", str);
        static StaticJsonDocument<max_size> doc;
        doc.clear();
        doc["log"] = str.c_str();
        if (doc.overflowed())
            mstd::fpr("webfpr_lager json overflowed");


        static char buffer[max_size];
        size_t len = serializeJson(doc, buffer, max_size);
        ws_server.textAll(buffer, len);
    }




    inline mstd::call_once ws_init(
        []() {
            // 配置 WebSocket 事件处理
            ws_server.onEvent([&](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
                if (type == WS_EVT_CONNECT) {
                    fpr("WebSocket 客户端", client->id(), "已连接\n");

                    {
                        JsonDocument doc;
                        JsonObject root = doc.to<JsonObject>();
                        root.createNestedArray("data");// 创建data数组

                        for (auto& [k, m] : mesp::wsValue_state)
                            m.get_json(doc);// 添加当前值到data数组
                        mesp::sendJson(doc);// 发送所有注册的值
                    }//之后应包装一下@_@

                } else if (type == WS_EVT_DISCONNECT) {
                    fpr("WebSocket 客户端 ", client->id(), "已断开\n");
                } else if (type == WS_EVT_DATA) {// 处理接收到的数据
                    // fpr("收到ws数据,WS_EVT_DATA");
                    data[len] = 0;// 确保字符串终止

                    JsonDocument doc;
                    deserializeJson(doc, data);
                    // fpr("ws收到的json\n", doc, "\n");

                    {
                        if (doc.containsKey("ping")) {
                            JsonDocument resp_doc;
                            JsonObject root = resp_doc.to<JsonObject>();
                            root["pong"] = doc["ping"].as<uint64_t>();
                            mesp::sendJson(resp_doc, *server);
                        } else//非心跳包
                            fpr("ws收到的json\n", doc, "\n");

                    }//心跳部分

                    {
                        if (doc.containsKey("data") && doc["data"].is<JsonArray>()) {
                            for (JsonObject obj : doc["data"].as<JsonArray>()) {
                                if (obj.containsKey("name")) {
                                    key_type name = obj["name"].as<const char*>();
                                    auto it = mesp::wsValue_state.find(name);
                                    if (it != mesp::wsValue_state.end())
                                        std::get<1>(*it).set_value(obj);//更新值
                                    else
                                        fpr("未知字段:", name);
                                }
                            }
                        }//wsvalue更新部分
                    }

                    {
                        Exstring<32> command = doc["action"]["command"] | Exstring<32>("_null");
                        if (command != "_null") {//处理命令json,每次只能单条
                            fpr("ws命令:", command);
                            auto it = command_state.find(command);
                            if (it != command_state.end())
                                std::get<1> (*it)(doc);
                            else
                                fpr("未知命令:", command);
                        }//if command
                    }

                }//WS_EVT_DATA
            });
        });// ws_init



}// namespace mesp

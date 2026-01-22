#pragma once

#include <functional>
#include <map>
#include <string>

#include "channel.hpp"
#include "esptools.hpp"
#include <ArduinoJson.hpp>
#include <ESPAsyncWebServer.h>


namespace mesp {

    inline std::map<std::string, std::function<void(const JsonObject&)>> ws_value_update;
    inline std::map<std::string, std::function<void(JsonDocument&)>> ws_value_to_json;
    //全局的ws服务
    inline AsyncWebSocket ws_server("/ws");//现在似乎也没有使用非全局的ws的需求,就先统一为这个了
    inline ConfigStore ws_config("ws");

    //发送json到前端
    inline void sendJson(const JsonDocument& doc, AsyncWebSocket& ws = ws_server) {
        String msg;
        serializeJson(doc, msg);
        ws.textAll(msg);
    };

    //与前端的同步类型
    template <typename T>
    struct wsValue {
        //@_@静态断言,T只能基本类型
        using value_type = T;

        const std::string name;//值名,不可改变
        value_type value;
        mstd::lock_key key;//互斥体,用于保护value的读写

        template <typename... V>
        wsValue(const std::string_view& n, V&&... v) : name(n), value(value_type(std::forward<V>(v)...)) {
            ws_value_update[name] = [this](const JsonObject& obj) {
                this->set_value(obj);
            };
            ws_value_to_json[name] = [this](JsonDocument& doc) {
                this->to_json(doc);
            };
            //注册到全局map中
            //可以考虑加个检查,如果有重复的就报错@_@
            //如果要扩展为非全局,可以给构造加一个入参,成员加一个引用
        }
        ~wsValue() {
            ws_value_update.erase(name);
            ws_value_to_json.erase(name);
        }

        wsValue(const wsValue&) = delete;
        wsValue& operator=(const wsValue&) = delete;
        wsValue(wsValue&&) = delete;
        wsValue& operator=(wsValue&&) = delete;
        //先关掉所有拷贝移动

        value_type& get_value() noexcept {
            mstd::RAII_lock k(key);
            return value;
        }
        const value_type& get_value() const noexcept {
            mstd::RAII_lock k(const_cast<mstd::lock_key&>(key));
            return value;
        }
        void set_value(const value_type& v) {
            mstd::RAII_lock k(key);
            value = v;
        }
        void set_value(value_type&& v) noexcept {
            mstd::RAII_lock k(key);
            value = std::move(v);
        }

        operator const value_type() const noexcept {
            return get_value();
        }

        void set_value(const JsonObject& obj) {//非基本类型,就需要特化这个成员函数
            if (!obj.isNull()) {//json为空就只更新前端状态的意思
                set_value(obj["value"].as<value_type>());
            }
            update();
        }

        void to_json(JsonDocument& doc) const {//非基本类型,就需要特化这个成员函数
            //构造对应的json
            //ArduinoJson想要分块构造貌似只能这样传?
            JsonArray data = doc["data"].as<JsonArray>();
            JsonObject item = data.createNestedObject();
            item["name"] = name;
            value_type val = get_value();
            item["value"] = val;
        }

        //构造对应的json且发送
        void update() const {
            JsonDocument doc;
            JsonObject root = doc.to<JsonObject>();
            root.createNestedArray("data");// 创建data数组

            to_json(doc);// 添加当前值到data数组
            sendJson(doc);
        }



        wsValue& operator=(const value_type& v) {
            set_value(v);
            update();
            return *this;
        }
        wsValue& operator=(value_type&& v) noexcept {
            set_value(std::move(v));
            update();
            return *this;
        }


        bool operator==(const value_type& v) const noexcept {
            return get_value() == v;
        }
    };//ws_value

    template <typename T>
    inline std::ostream& operator<<(std::ostream& os, const wsValue<T>& v) {
        return os << v.get_value();
    }


    //与前端的同步类型,会写硬盘
    template <typename T>
    struct wsStoreValue : wsValue<T> {

        using typename wsValue<T>::value_type;
        using wsValue<T>::name;
        using wsValue<T>::value;
        using wsValue<T>::get_value;
        using wsValue<T>::set_value;
        using wsValue<T>::update;

        template <typename... V>
        wsStoreValue(const std::string_view& n, V&&... v) : wsValue<T>(n, std::forward<V>(v)...) {
            // 从配置中加载初始值，如果没有则使用传入的默认值
            wsValue<T>::set_value(ws_config.get(name, get_value()));
            ws_value_update[name] = [this](const JsonObject& obj) {
                this->wsStoreValue::set_value(obj);
            };
            // if(n.size()>15)
            // fpr("error: 键名过长 ",n);
            fpr("键名:", n, " ", name);
        }


        void set_value(const JsonObject& obj) {//非基本类型,就需要特化这个成员函数
            if (!obj.isNull()) {//为空就只更新前端状态的意思
                wsValue<T>::set_value(obj["value"].as<value_type>());//1.这里已经有update()
                ws_config.set(name, get_value());
            } else//2.所以要else
                update();
        }


        wsStoreValue& operator=(const value_type& v) {
            set_value(v);
            ws_config.set(name, get_value());
            return *this;
        }
        wsStoreValue& operator=(value_type&& v) noexcept {
            set_value(std::move(v));
            ws_config.set(name, get_value());
            return *this;
        }

    };//ws_value

    //外面加到两个map后其实set_value的设计有点多余了,还有get_value(),@_@之后修改



    /*
    wsValue<类型(只支持基本类型)> a("键名",默认值);
    键名是和前端的name对应
    如果是wsStoreValue,同时也是存falsh的键名
    */

}//mesp

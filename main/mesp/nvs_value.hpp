/*
* Copyright (C) 2025-2026 by nccrrv
* SPDX-License-Identifier: AGPL-3.0-or-later
* 
* nvs_value.hpp
* 自动存入flash同步的值类型
*
* Example:
*   nvs_value<值的类型> my_value{键名, nvs里没有时的默认值};
*   nvs_value<uint32_t> my_value{"my_key", 0};
*   uint32_t val = my_value.get(); // 从NVS获取值
*   my_value = 42; // 将值写入NVS
*/

#pragma once
#include "../mstd/Exstring.hpp"

#include <nvs.h>
#include <nvs_flash.h>

namespace mesp {
    using mstd::Exstring;
    using mstd::fpr;
    using mstd::log_error;
    using mstd::log_info;

    namespace details {

        inline mstd::call_once __init_nvs(
            []() {
                auto err = nvs_flash_init();
                // 如果NVS分区损坏或首次使用，需要擦除并重新初始化
                if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
                    ESP_ERROR_CHECK(nvs_flash_erase());// 擦除NVS分区
                    err = nvs_flash_init();// 重新初始化NVS
                }
                ESP_ERROR_CHECK(err);
            });

    }//details

    struct nvs_space {
        nvs_handle_t handle;
        nvs_space(const char* namespace_name, nvs_open_mode_t open_mode = NVS_READWRITE) {
            esp_err_t err = nvs_open(namespace_name, open_mode, &handle);
            if (err != ESP_OK) {
                mstd::log_error("打开NVS命名空间失败");
            }
        }
        ~nvs_space() {
            nvs_close(handle);
        }

        operator nvs_handle_t&() noexcept {
            return handle;
        }
    };


    using key_type = Exstring<15>;//NVS键名最大长度为15
    //话说这不是整数啊,应该是16,文档没算\0吧,之后再测试@_@

    inline nvs_space store_space{"store"};

    //自动在NVS中存取值
    template <typename T>
    //约束T是平凡类型
    struct nvs_value {

        using value_type = T;
        using const_value_type = const value_type;

        key_type key;
        value_type value;

        nvs_value(const key_type& k, const_value_type& def)
            : key(k), value(def) {
            size_t value_size = sizeof(value_type);
            auto err = nvs_get_blob(store_space, key.c_str(), &value, &value_size);

            if (err == ESP_ERR_NVS_NOT_FOUND) {//找不到键,需要初始化
                this->operator=(def);
            } else if (err != ESP_OK) {
                mstd::log_error("读取NVS失败");
                fpr("错误码:", esp_err_to_name(err));
            }
        }

        const_value_type& operator=(const_value_type& v) {
            auto err = nvs_set_blob(store_space, key.c_str(), &v, sizeof(value_type));

            if (err != ESP_OK) {
                mstd::log_error("写入NVS失败");
            } else {
                err = nvs_commit(store_space);
                if (err == ESP_OK) {
                    value = v;
                    mstd::log_info("成功写入 键: ", key, " 值: ", value);
                }
            }

            return value;
        }


        template <typename Y>
            requires requires(Y&& r) {
                value_type(std::forward<Y>(r));
                requires (!Meta::not_base_same<Y, value_type>);
            }
        const_value_type& operator=(Y&& r) {
            return this->operator=(value_type(std::forward<Y>(r)));
        }


        const_value_type& get() const noexcept {
            return value;
        }
        const_value_type& set(const value_type& v) {
            this->operator=(v);
            return value;
        }

        operator value_type&() noexcept {
            return value;
        }
        operator const_value_type&() const noexcept {
            return value;
        }


        template <typename Y>
        bool operator==(const Y& r) const noexcept {
            return value == r;
        }

    };//NVSvalue


    template <typename T, typename Y>
    inline bool operator==(const T& l, const mesp::nvs_value<Y>& r) noexcept {
        return l == r.get();
    }

    template <typename T>
    inline std::ostream& operator<<(std::ostream& os, const mesp::nvs_value<T>& v) {
        os << v.get();
        return os;
    }


}//mesp
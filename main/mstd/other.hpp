/*
* Copyright (C) 2025-2026 by nccrrv
* SPDX-License-Identifier: AGPL-3.0-or-later
* 
* other.hpp
* 其他工具函数和类的声明
*/

#pragma once
#include "Meta.hpp"

#include <chrono>
#include <iostream>
#include <source_location>
#include <thread>


namespace mstd {

    using uchar_t = unsigned char;


    template <typename T>
    inline constexpr void fpr(T&& x) {
        if constexpr (Meta::base_same<T, uint8_t> || Meta::base_same<T, int8_t>)
            std::cout << static_cast<int16_t>(std::forward<T>(x)) << std::endl;
        else if constexpr (Meta::base_same<T, bool>)
            std::cout << (x ? "TRUE" : "FALSE") << std::endl;
        else
            std::cout << std::forward<T>(x) << std::endl;
    }
    template <typename T, typename... V>
    inline constexpr void fpr(T&& a, V&&... v) {
        if constexpr (Meta::base_same<T, uint8_t> || Meta::base_same<T, int8_t>)
            std::cout << static_cast<int16_t>(std::forward<T>(a));
        else if constexpr (Meta::base_same<T, bool>)
            std::cout << (a ? "TRUE" : "FALSE") << std::endl;
        else
            std::cout << std::forward<T>(a) /*<< ' '*/;//2023年4月13日22:40:58取消空格,认为格式应该由外界控制
        fpr(std::forward<V>(v)...);
    }

#define fpr_value(x) fpr(#x, ':', (x))

#if defined(__GXX_RTTI) || defined(_CPPRTTI)
    //打印类型
    template <typename T>
    inline constexpr void fpr() {
        if constexpr (Meta::is_rref<T>)
            fpr(typeid(T).name(), " &&");
        else if (std::is_reference_v<T> && std::is_const_v<Meta::rm_ref<T>>)
            fpr("const ", typeid(T).name(), " &");
        else if (std::is_const_v<T>)
            fpr("const ", typeid(T).name());
        else if (std::is_reference_v<T>)
            fpr(typeid(T).name(), " &");
        else
            fpr(typeid(T).name());
        //因为typeid认不出const,&,&&
    }
#endif

#ifdef NO_LOG
#define NO_LOG_INFO
#define NO_LOG_ERROR
#endif

    template <typename... V>
    inline constexpr void log_info(V&&... v) {
#ifdef NO_LOG_INFO
        return;
#else
        fpr("info: ", std::forward<V>(v)...);
#endif
    }

    template <typename T>
    inline constexpr void log_error(T&& msg, const std::source_location& loc = std::source_location::current()) {
        fpr("error: ", std::forward<T>(msg), " at ", loc.file_name(), ":", loc.line(), " in ", loc.function_name());
    }





    template <typename T>
    inline void delay(const T& t) { std::this_thread::sleep_for(t); }

    template <typename T>
    inline void delay_ms(const T& t) { std::this_thread::sleep_for(std::chrono::milliseconds(t)); }


    struct call_once {
        template <typename F>
        //约束,可执行对象,无参@_@
        call_once(const F& f) { f(); }
    };

}//mstd


using namespace std::chrono_literals;


#ifdef _WIN32

#define NOMINMAX// 禁止 min 和 max 宏
#include <windows.h>

inline mstd::call_once __set_utf8([]() {
    system("chcp 65001");
});

#endif//_WIN32
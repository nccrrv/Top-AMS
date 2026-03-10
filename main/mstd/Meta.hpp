/*
* Copyright (C) 2025-2026 by nccrrv
* SPDX-License-Identifier: AGPL-3.0-or-later
* 
* Meta.hpp
* 元编程库
*/

#pragma once

#include <assert.h>
#include <concepts>
#include <cstdint>
#include <tuple>
#include <type_traits>



namespace Meta {//声明与说明

    /*
	* 一些只靠特化的空"基"类会被顺便定义
	* is_XX为模板类,确认T是否为XX,没有额外功能
	* XX_type为约束,为了方便放在template中,同时会有移除引用const
	*/

    using std::false_type;
    using std::size_t;
    using std::true_type;

    //一些标准库元函数的缩写

    //将类型增加左值引用
    template <typename T>
    using lref = typename std::add_lvalue_reference<T>::type;
    //将类型增加右值引用
    template <typename T>
    using rref = typename std::add_rvalue_reference<T>::type;
    //将类型移除左值或右值引用
    template <typename T>
    using rm_ref = typename std::remove_reference<T>::type;
    //将类型移除const
    template <typename T>
    using rm_c = typename std::remove_const<T>::type;
    //同时移除引用和const,必须先移除引用才行
    template <typename T>
    //using rm_cref = rm_c<rm_ref<T>>;
    using rm_cref = std::remove_cvref<T>::type;
    //注意字符串进来是cosnt char[],这里的cosnt会被移除
    //移除一层指针,隐式移除引用和顶const
    template <typename T>
    using rm_p = std::remove_pointer<rm_ref<T>>::type;

    //元if,布尔判断返回类型
    template <bool B, typename T, typename F>
    using if_meta = typename std::conditional<B, T, F>::type;
    //包装特定类型的编译期常量
    template <auto V>
    using Value = typename std::integral_constant<decltype(V), V>;
    //感觉标准库的这个功能有点少,可能需要扩展

    //检查对象是否是左值引用
    template <typename T>
    inline constexpr bool is_lref = std::is_lvalue_reference_v<T>;
    //检查对象是否是右值引用
    template <typename T>
    inline constexpr bool is_rref = std::is_rvalue_reference_v<T>;


    namespace detail {
        template <typename T, typename Y>
        concept __base_same = std::is_same_v<rm_cref<T>, rm_cref<Y>>;
    }//detail

    //确定类型基础上是同一类型(移除const和引用
    template <typename T, typename... Y>
    concept base_same = (detail::__base_same<T, Y> && ...);

    template <typename T, typename... Y>
    concept not_base_same = !(detail::__base_same<T, Y> || ...);


    template <typename T>
    struct get_value_type {
        using type = T;
    };
    template <typename T>
        requires requires { typename T::value_type; }
    struct get_value_type<T> {
        using type = typename T::value_type;
    };

    template <typename T>
    using get_value_type_t = typename get_value_type<T>::type;


    template <typename T>
        requires requires { std::declval<T>().c_str(); }
    inline constexpr const char* get_c_str(T&& t) noexcept {
        return std::forward<T>(t).c_str();
    }
    inline constexpr const char* get_c_str(const char* t) noexcept {
        return t;
    }
    template <size_t N>
    inline constexpr const char* get_c_str(const char (&t)[N]) noexcept {
        return t;
    }

    template <typename T>
        requires requires { std::declval<T>().size(); }
    inline constexpr auto get_size(T&& t) noexcept {
        return std::forward<T>(t).size();
    }
    //获取字符指针指向内容的长度
    inline constexpr size_t get_size(const char* t) noexcept {
        size_t i = 0;
        while (t[i] != 0)
            ++i;
        return i;
    }
    template <size_t N>
    inline constexpr size_t get_size(const char (&t)[N]) noexcept {
        return N - 1;
    }


}//Meta
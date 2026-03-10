/*
* Copyright (C) 2025-2026 by nccrrv
* SPDX-License-Identifier: AGPL-3.0-or-later
* 
* Exstring.hpp
* 尽可能减少内存碎片为目的设计的字符串库
*/

#pragma once

#include "rangebase.hpp"
#include <charconv>


namespace mstd {


    struct Exstring_base : rangebase {

        using value_type = char;
        using point_type = value_type*;
        using const_point_type = const value_type*;

        constexpr const_point_type c_str(this auto&& This) noexcept {
            return This.data();
        }

        using rangebase::operator==;

        template <size_t N>
        constexpr bool operator==(this const auto& This, const value_type (&r)[N]) noexcept {
            if (This.size() != N - 1)
                return false;
            for (size_t i = 0; i < N; i++) {
                if (This[i] != r[i])
                    return false;
            }
            return true;
        }

    };//Exstring_base


    template <size_t... N>
    struct Exstring;

    template <typename T>
    struct is_Exstring : std::false_type {};
    template <size_t... N>
    struct is_Exstring<Exstring<N...>> : std::true_type {};
    template <typename T>
    inline constexpr bool is_Exstring_v = is_Exstring<T>::value;
    template <typename T>
    concept Exstring_type = is_Exstring_v<Meta::rm_cref<T>>;


    template <>
    struct Exstring<> : Exstring_base {
        std::vector<value_type> _data{};

        constexpr Exstring() = default;


        //@_@后面有空再写
        //主要是干啥都要扩容,需要更好的堆基础设施
        //+的表达式模板
    };//Exstring<>

    template <typename T>
    struct to_Exstring_size {
        static_assert(false, "to_Exstring_size is not specialized for this type");
    };
    template <>
    struct to_Exstring_size<int8_t> {
        constexpr static size_t value = 5;
    };
    template <>
    struct to_Exstring_size<uint8_t> {
        constexpr static size_t value = 4;
    };
    template <>
    struct to_Exstring_size<int16_t> {
        constexpr static size_t value = 7;
    };
    template <>
    struct to_Exstring_size<uint16_t> {
        constexpr static size_t value = 6;
    };
    template <>
    struct to_Exstring_size<int32_t> {
        constexpr static size_t value = 12;
    };
    template <>
    struct to_Exstring_size<uint32_t> {
        constexpr static size_t value = 11;
    };
    template <>
    struct to_Exstring_size<int64_t> {
        constexpr static size_t value = 21;
    };
    template <>
    struct to_Exstring_size<uint64_t> {
        constexpr static size_t value = 21;
    };
    template <size_t N>
    struct to_Exstring_size<const char[N]> {
        constexpr static size_t value = N;
    };
    template <size_t N>
    struct to_Exstring_size<char[N]> {
        constexpr static size_t value = N;
    };

    // 浮点类型后面再补@_@,位数这些要确定一下,有扩展系数

    template <typename T>
    inline constexpr size_t to_Exstring_v = to_Exstring_size<Meta::rm_cref<T>>::value;



    template <size_t N>
    struct Exstring<N> : Exstring_base {
        constexpr static size_t _max_size = N;
        // private:
        std::array<value_type, N> _data{};
        size_t _size = 0;

      public:
        constexpr Exstring() = default;
        constexpr Exstring(const Exstring&) = default;

        template <size_t M>
        constexpr Exstring(const value_type (&S)[M]) {
            static_assert(M <= N, "stack_string overflow");
            for (std::make_signed_t<size_t> i = 0; i < M - 1; i++)// 结尾的'\0'无须拷贝
                _data[i] = S[i];
            _size = M - 1;
        }

        //过长会直接截断
        constexpr Exstring(const value_type* S) {
            size_t i = 0;
            for (; i < N - 1; i++) {
                if (S[i] == '\0')
                    break;
                _data[i] = S[i];
            }
            _size = i;
        }

        //过长会直接截断
        constexpr Exstring(const value_type* S, size_t len) {
            size_t i = 0;
            for (; i < std::min(len, N - 1); i++)
                _data[i] = S[i];
            _size = i;
        }

        template <size_t M>
        // requires (M <= N)
        constexpr Exstring(const Exstring<M>& r) {
            static_assert(M <= N, "stack_string overflow");
            for (size_t i = 0; i < r._size; i++)// 结尾的'\0'无须拷贝
                _data[i] = r[i];
            _size = r._size;
        }

        template <typename Y>
            requires std::is_arithmetic_v<Meta::rm_cref<Y>> &&
                     (Meta::not_base_same<Y, bool>) &&
                     (!Exstring_type<Y>)
        constexpr Exstring(Y&& v) {
            auto [p, ec] = std::to_chars(data(), data() + max_size(), std::forward<Y>(v));
            _size = p - data();
            if (ec != std::errc())
                fpr("stack_string error: ", v, " ", std::make_error_code(ec).message());
        }


        constexpr decltype(auto) data(this Meta::base_same<Exstring> auto&& This) noexcept {
            return This._data.data();
        }

        constexpr size_t size() const noexcept {
            return _size;
        }
        constexpr static size_t max_size() noexcept {
            return N;
        }

        constexpr Exstring<N + 1> operator-() const
            noexcept {// 用于数字类型的字符串,反正其他类型也用不到负号,这里就不套一层代理类了
            Exstring<N + 1> res;
            res[0] = '-';
            for (size_t i = 0; i < _size; i++)
                res[i + 1] = _data[i];
            res._size = _size + 1;
            return res;
        }

        template <size_t M>
        constexpr Exstring<N + M - 1> operator+(const Exstring<M>& other) const noexcept {
            Exstring<N + M - 1> res;
            for (size_t i = 0; i < _size; i++)
                res[i] = _data[i];
            for (size_t i = 0; i < other._size; i++)
                res[i + _size] = other._data[i];
            res._size = _size + other._size;
            return res;
        }

        template <typename T>
            requires (!Exstring_type<T>)
        constexpr auto operator+(T&& r) const noexcept {
            return *this + Exstring<to_Exstring_v<T>>(std::forward<T>(r));
        }


    };//Exstring<N>

    template <size_t N>
    Exstring(const char (&)[N]) -> Exstring<N>;

    template <typename T>
        requires (!std::is_pointer_v<T>)
    Exstring(T) -> Exstring<to_Exstring_size<Meta::rm_cref<T>>::value>;


    template <typename T, size_t N>
        requires (!Exstring_type<T>)
    constexpr auto operator+(T&& l, const Exstring<N>& r) noexcept {
        return Exstring<to_Exstring_v<T>>(std::forward<T>(l)) + r;
    }

    template <size_t N, size_t M>
    constexpr bool operator==(const char (&l)[N], const Exstring<M>& r) noexcept {
        return r == l;
    }

    template <size_t... N>
    std::ostream& operator<<(std::ostream& os, const Exstring<N...>& ss) {
        os << ss.c_str();
        return os;
    }



}//mstd
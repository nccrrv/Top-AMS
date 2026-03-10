/*
* Copyright (C) 2025-2026 by nccrrv
* SPDX-License-Identifier: AGPL-3.0-or-later
* 
* rangebase.hpp
* 范围基础类的声明
*/

#pragma once

#include "Meta.hpp"
#include "other.hpp"
#include <compare>

namespace mstd {

    struct rangebase {

        //   private:
        constexpr decltype(auto) data(this auto&& This) noexcept {
            return This.data();
        }

        constexpr decltype(auto) size(this const auto& This) noexcept {
            return This.size();
        }

      public:
        template <typename T>
        constexpr decltype(auto) operator[](this T&& This, size_t index) noexcept {
            return std::forward_like<T>(This.data()[index]);
        }

        constexpr decltype(auto) begin(this auto&& This) noexcept {
            return This.data();
        }
        constexpr decltype(auto) end(this auto&& This) noexcept {
            return This.data() + This.size();
        }

        constexpr bool empty(this const auto& This) noexcept {
            return This.size() == 0;
        }


        //比较符号类目前不限制两边类型

        constexpr bool operator==(this const auto& This, const auto& r) noexcept {
            if (This.size() != r.size())
                return false;
            for (size_t i = 0; i < This.size(); i++) {
                if (This[i] != r[i])
                    return false;
            }
            return true;
        }

        constexpr std::strong_ordering operator<=>(this const auto& This, const auto& r) noexcept {
            size_t min_size = This.size() < r.size() ? This.size() : r.size();
            for (size_t i = 0; i < min_size; i++) {
                if (This[i] < r[i])
                    return std::strong_ordering::less;
                else if (This[i] > r[i])
                    return std::strong_ordering::greater;
            }
            return This.size() <=> r.size();
        }

    };//range_base


}//mstd;
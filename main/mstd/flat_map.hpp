/*
* Copyright (C) 2025-2026 by nccrrv
* SPDX-License-Identifier: AGPL-3.0-or-later
* 
* flat_map.hpp
* 简单的手搓flat_map,用于C++23库文件支持不全的地方
*/

#pragma once

#include <tuple>
#include <vector>


namespace mstd {
    using std::tuple;
    using std::vector;

    template <typename KEY, typename MAP>
    struct flat_map {

        using key_type = KEY;
        using mapped_type = MAP;
        using pair_type = std::tuple<key_type, mapped_type>;

        std::vector<pair_type> map;

        flat_map(size_t n = 0) {
            map.reserve(n);
        }

        template <typename T, typename Y>
        constexpr void emplace(T&& k, Y&& m) {
            auto target = std::ranges::lower_bound(map, k, std::ranges::less{}, [](auto const& p) -> const auto& {
                return std::get<0>(p);
            });
            if (target != end() && std::get<0>(*target) == k) {
                std::get<1>(*target) = std::forward<Y>(m);
                return;
            }

            map.insert(target, pair_type(std::forward<T>(k), std::forward<Y>(m)));
        }//emplace

        constexpr auto find(this auto&& This, const key_type& k) {
            auto it = std::ranges::lower_bound(This.map, k, std::ranges::less{}, [](auto const& p) -> const auto& {
                return std::get<0>(p);
            });
            if (it != This.map.end() && std::get<0>(*it) == k)
                return it;
            return This.map.end();
        }//find

        constexpr auto operator[](this auto&& This, const key_type& k) -> mapped_type& {
            auto target = This.find(k);
            if (target != This.map.end()) {
                return std::get<1>(*target);
            }
            This.emplace(k, mapped_type{});//当然,前提是支持默认构造,这里或许可以考虑搞个模板偏序处理
            return This[k];
        }

        constexpr auto begin(this auto&& This) noexcept {
            return This.map.begin();
        }
        constexpr auto end(this auto&& This) noexcept {
            return This.map.end();
        }

        constexpr void erase(const key_type& k) {
            auto target = find(k);
            if (target != map.end()) {
                map.erase(target);
            }
        }

    };//flat_map

}//mstd
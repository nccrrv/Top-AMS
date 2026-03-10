/*
* Copyright (C) 2025-2026 by nccrrv
* SPDX-License-Identifier: AGPL-3.0-or-later
* 
* channel.hpp
* esp用的通道类
* 
* Example:
*   channel<int,10> c;//创建一个长度为10的int通道
*   c.emplace(4);//插入一个元素,值为4
*   int v=c.pop();//获取(弹出)通道里元素,如果通道为空,则会阻塞
*/

#pragma once

#include "../mstd/other.hpp"
#include "freertos/queue.h"
#include "freertos/task.h"


namespace mesp {

    template <typename T, size_t N>
    //当前要求T是POD,且不应该过大,否则指针效率更高
    struct channel {
        using value_type = T;
        static constexpr size_t max_size = N;

        StaticQueue_t xStaticQueue;
        std::array<value_type, max_size> QueueStorage;

        QueueHandle_t queue = xQueueCreateStatic(
            max_size,
            sizeof(value_type),
            reinterpret_cast<uint8_t*>(QueueStorage.data()),
            &xStaticQueue);

        channel() = default;
        ~channel() {
            if (queue) {
                vQueueDelete(queue);
            }
        }

        //插入元素,满了失败,不插入
        bool emplace(const value_type& v) {
            xQueueSend(queue, &v, 0);
            return true;
            // return xQueueSend(queue, &v, 0) == pdTRUE ? true, false;
        }

        //插入元素,满了覆盖
        void emplace_overwrite(const value_type& v) {
            auto res = xQueueSend(queue, &v, 0);
            while (res != pdTRUE) {
                value_type discard;
                xQueueReceive(queue, &discard, 0);
                res = xQueueSend(queue, &v, 0);
            }//不带锁了,反正都是丢
        }

        //弹出元素,为空会阻塞
        void pop(value_type& v) {
            xQueueReceive(queue, &v, portMAX_DELAY);
        }

        value_type pop() {
            value_type v{};//要求支持默认构造,否则要用点东西
            pop(v);
            return v;
        }

        //获取通道已有元素
        auto size() const {
            return uxQueueMessagesWaiting(queue);
        }

        //带关闭的再说,需要sizeflag

    };//channel


}//mesp

#pragma once
#include "freertos/FreeRTOS.h"
#include "mstd/other.hpp"

inline void fpr_mem() {
    mstd::delay(200ms);//这个内存打印不是很实时
    heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
}
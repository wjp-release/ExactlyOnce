#pragma once

#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cstdint>

#include <functional>
#include <stdexcept>
#include <iostream>
#include <memory>

typedef int64_t  sb8;
typedef int32_t  sb4;
typedef int16_t  sb2;
typedef int8_t   sb1;
typedef uint64_t ub8;
typedef uint32_t ub4;
typedef uint16_t ub2;
typedef uint8_t  ub1;

#include "perfreak.h"

namespace wjp{
    // 全局常量
    const static int kPageSize = 4096;


}
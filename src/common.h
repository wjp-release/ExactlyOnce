#pragma once

#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cstdint>

#include <memory>


// WJP C++ Style
// 默认使用Google Style Guide，除了下列不同：
// 1. 但方法首字母小写。
// 2. 文件名尽可能简洁而精准，小写。 
// 3. 类型名尽可能简洁而精准，class与struct用驼峰，基础类型和内部struct小写。
// 4. 成员变量用首字母小写的驼峰，不加下划线后缀。
// 5. 优先用下面一族类型名做整型类型。
// 6. 只在必要时使用c++的cast，其他时候沿用c的cast。
// 7. 以后用中文写注释。
typedef int64_t  sb8;
typedef int32_t  sb4;
typedef int16_t  sb2;
typedef int8_t   sb1;
typedef uint64_t ub8;
typedef uint32_t ub4;
typedef uint16_t ub2;
typedef uint8_t  ub1;

// 64位机器，默认8字节对齐。
#define ALIGN(x) (((x)+(ub8)(7u)) & ~ (ub8)(7u))
// 避免64位数字字面量导致编译器警告，用宏拼起来。
#define UB8(high, low) ((ub8)(high)<<32 | (ub8)(low))
// 64位机器的虚拟地址只用了低48位，高16位可用于额外存储。
// PTR_SET可将一个ub8类型的val（如果不足8字节，则需要自己左移，制造一个ub8）的前16位存入ptr。
#define PTR_SET16(type, ptr, val) (ptr = (type*)((ub8)(p) & (ub8)(UB8(0xffff0000, 0x00000000)) | (ub8)(val) ))
// PTR_GET可将一个存了额外信息的ptr的前16位的数据清空，成为合法的指针。
#define PTR_GET48(type, ptr) ( (type*) ( (ub8)(ptr) & (ub8)(UB8(0x0000ffff, 0xffffffff))) )


namespace wjp{
    // 全局常量
    const static int kPageSize = 4096;


}
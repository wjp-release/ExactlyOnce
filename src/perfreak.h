#pragma once

// 64位机器，默认8字节对齐。
#define ALIGN(x) (((x)+(ub8)(7u)) & ~ (ub8)(7u))

// 避免64位数字字面量导致编译器警告，用宏拼起来。
#define UB8(high, low) ((ub8)(high)<<32 | (ub8)(low))

namespace wjp{
// 更高倍数的对齐，对cache更友好，尤其是64字节对齐，处于cache line开头。
// 除此之外，对齐的指针的低位空着，还能存点额外数据。
static inline char* malloc64(ub4 size){
    void* p;
    if (!posix_memalign(&p, 64, size)) return (char*)p;
    else return nullptr;
}

static inline char* malloc32(ub4 size){
    void* p;
    if (!posix_memalign(&p, 32, size)) return (char*)p;
    else return nullptr;
}

static inline char* malloc16(ub4 size){
    void* p;
    if (!posix_memalign(&p, 16, size)) return (char*)p;
    else return nullptr;
}

// 如果认定99%可能为true，再用likely，否则不见得比默认分支预测强。
static inline bool likely(bool x){
#if defined(__GNUC__) || defined(__clang__) 
    return __builtin_expect(!!(x), 1);
#else
    return x;
#endif
}

static inline bool unlikely(bool x){
#if defined(__GNUC__) || defined(__clang__) 
    return __builtin_expect(!!(x), 0);
#else
    return x;
#endif
}

// 下面是一组利用64位系统虚拟地址前16位必然为空的操作，把这16位用作额外存储。
// 高16位（原本为空）赋值为value。
template < typename T >
static inline T* assign16(T* ptr, ub2 value){
    return (T*) ((ub8)ptr | ( (ub8)(value) << 48));
}

// 低48位赋值为newptr。
template < typename T, typename U >
static inline T* assign48(T* ptr, U* newptr){
    return (T*) (((ub8)ptr & UB8(0xffff0000, 0x00000000)) | (ub8)(newptr));
}

// 清空高16位。
template < typename T > 
static inline T* clear16(T* ptr){
    return (T*) ((ub8)ptr & UB8(0x0000ffff, 0xffffffff));
}

// 得到ptr的前16位，转成ub2。
template < typename T >
static inline ub2 get16(T* ptr){
    return (ub2) ((ub8)(ptr) >> 48);
}


}
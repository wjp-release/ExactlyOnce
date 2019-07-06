#pragma once

#include "common.h"

namespace wjp{

class Arena{
public:
    static const int kSmallShift = 3; // 判断是否单独分配chunk的启发式线索
    
    // Arena的chunk默认大小为4页，需根据具体应用调整。
    Arena(ub4 chunkCapacity = 4*kPageSize): chunkCapacity(chunkCapacity){}

    // Arena不存在free接口，内存在Arena对象析构时统一回收。
    ~Arena(){
        while (currentChunk){
            auto tofree=currentChunk;
            currentChunk = currentChunk->next;
            std::free(tofree);
        }
    }

    // 分配规则如下：
    // 1. 若尚不存在链表，则新建chunk。
    // 2. 溢出时，对大型对象独立分配等尺寸chunk，链入链表第二位。
    // 3. 对其他对象分配常规尺寸chunk，把对象放入新chunk开头，并将新chunk替换为链表头。
    char* alloc(ub4 size){
        if (!size) return nullptr;
        size = ALIGN(size);
        if (!currentChunk || currentSize + size > chunkCapacity){
            if (size < (chunkCapacity >> kSmallShift)){
                if (auto p = malloc64(chunkCapacity)){
                    chunk* new_chunk = new(p) chunk;
                    new_chunk->next  = currentChunk;
                    currentChunk     = new_chunk;
                    currentSize      = size + kChunkSize;
                    return p + kChunkSize;
                }else return nullptr;
            }else{
                if (!currentChunk){
                    if (auto p = malloc64(chunkCapacity)){
                        currentChunk    = new(p) chunk;
                        currentSize     = kChunkSize;
                    }else return nullptr;
                }
                if (auto p = malloc64(size + kChunkSize)){
                    chunk* new_chunk    = new(p) chunk;
                    new_chunk->next     = currentChunk->next;
                    currentChunk->next  = new_chunk;
                    return p + kChunkSize;
                }else return nullptr;
            }
        }else{
            auto p = currentPointer();
            currentSize += size;
            return p;
        }
    }

    // 空间增长规则如下：
    // 1. 禁止缩小。
    // 2. 传入的旧指针若为nullptr，则视为一次新的alloc。
    // 3. 传入的旧指针若为最近分配的那个，则尝试直接利用后续空间。
    // 4. 后续空间不足或并非最近分配的指针，则重新alloc并简单复制。
    char* grow(char* oldptr, ub4 oldlen, ub4 newlen){
        if (!oldptr) return alloc(newlen);
        if (!newlen) return nullptr;
        if (newlen <= oldlen) return oldptr;
        oldlen = ALIGN(oldlen), newlen = ALIGN(newlen);
        if (oldptr + oldlen == currentPointer()){
            if (currentSize + newlen - oldlen <= chunkCapacity){
                currentSize += newlen - oldlen;
                return oldptr;
            }
        }
        if (char* newptr = alloc(newlen)){
            if (oldlen) std::memcpy(newptr, oldptr, oldlen);
            return newptr;
        }else return nullptr;
    }

private:
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    inline char* currentPointer(){ return (char*)currentChunk + currentSize; }

    struct chunk{
        chunk* next = 0;
    };

    static const ub8 kChunkSize = ALIGN(sizeof(chunk)); 

    ub4         currentSize = 0;
    chunk*      currentChunk = 0;
    ub4         chunkCapacity; 
};


// 为了避免妥协Arena代码的简洁性、健壮性，这里单独实现一个它的改造版本。
// UserBufferArena要求用户提供自己的缓冲区，用完了之后再以Arena的规则进行内存分配。
// 它对Arena的内存布局看似完全未修改，但实际上利用了64位系统虚拟地址高16位为空，
// 让currentChunk指针额外存储用户缓冲区的大小，并以此判断当前是否在用user buffer。
class UserBufferArena{
protected:
    // 若user buffer capacity不为0，即表示正在用user buffer。
    inline ub2 userBufferCapacity(){
        return get16(currentChunk);
    }    

    inline char* userBufferAddress(){
        return (char*)clear16(currentChunk);
    }

public:
    static const int kSmallShift = 3; // 判断是否单独分配chunk的启发式线索
    
    // 
    UserBufferArena(char* userBuffer, ub2 userBufferSize, ub4 chunkCapacity = 4*kPageSize) : chunkCapacity(chunkCapacity)
    {
        currentChunk = (chunk*) userBuffer;
        currentChunk = assign16(currentChunk, userBufferSize);
    }   

    // 使用与Arena一致的构造函数，则UserBufferArena的行为会与Arena一致。
    UserBufferArena(ub4 chunkCapacity = 4*kPageSize): chunkCapacity(chunkCapacity){}

    // 用户缓冲区无需free，因此和Arena的析构恰好一致。
    ~UserBufferArena(){
        while (currentChunk){
            auto tofree=currentChunk;
            currentChunk = currentChunk->next;
            std::free(tofree);
        }
    }

    // 分配规则：
    // 1. 检验是否正在用user buffer，未用则行为如Arena::alloc
    // 2. 若在用user buffer，且未溢出，则直接移动currentSize
    // 3. 溢出，则将currentChunk和currentSize均置为0，
    //    使之处于Arena初始态并继续以Arena::alloc初次分配的方式处理此次分配。
    //    用户缓冲就此舍弃。
    char* alloc(ub4 size){
        if (!size) return nullptr;
        size = ALIGN(size);
        // 先检验是否在使用user buffer。
        auto ubcap = userBufferCapacity();
        if (ubcap){
            if (currentSize + size <= ubcap){
                char* p = userBufferAddress() + currentSize;
                currentSize += size;
                return p;
            }else{
                currentChunk = nullptr;
                currentSize = 0;
            }
        }
        // 未使用user buffer，行为与Arena一致。
        if (!currentChunk || currentSize + size > chunkCapacity){
            if (size < (chunkCapacity >> kSmallShift)){
                if (auto p = malloc64(chunkCapacity)){
                    chunk* new_chunk = new(p) chunk;
                    new_chunk->next  = currentChunk;
                    currentChunk     = new_chunk;
                    currentSize      = size + kChunkSize;
                    return p + kChunkSize;
                }else return nullptr;
            }else{
                if (!currentChunk){
                    if (auto p = malloc64(chunkCapacity)){
                        currentChunk    = new(p) chunk;
                        currentSize     = kChunkSize;
                    }else return nullptr;
                }
                if (auto p = malloc64(size + kChunkSize)){
                    chunk* new_chunk    = new(p) chunk;
                    new_chunk->next     = currentChunk->next;
                    currentChunk->next  = new_chunk;
                    return p + kChunkSize;
                }else return nullptr;
            }
        }else{
            auto p = currentPointer();
            currentSize += size;
            return p;
        }
    }

    // 空间增长规则如下：
    // 1. 禁止缩小。
    // 2. 传入的旧指针若为nullptr，则视为一次新的alloc。
    // 3. 传入的旧指针若为最近分配的那个，则尝试直接利用后续空间。
    //    但这里需要注意根据是否在用user buffer决定capacity值。
    // 4. 后续空间不足或并非最近分配的指针，则重新alloc并简单复制。
    char* grow(char* oldptr, ub4 oldlen, ub4 newlen){
        if (!oldptr) return alloc(newlen);
        if (!newlen) return nullptr;
        if (newlen <= oldlen) return oldptr;
        oldlen = ALIGN(oldlen), newlen = ALIGN(newlen);
        if (oldptr + oldlen == currentPointer()){
            auto ubcap = userBufferCapacity();
            ub4 currentCapacity = ubcap ? ubcap : chunkCapacity;
            if (currentSize + newlen - oldlen <= currentCapacity){
                currentSize += newlen - oldlen;
                return oldptr;
            }
        }
        if (char* newptr = alloc(newlen)){
            if (oldlen) std::memcpy(newptr, oldptr, oldlen);
            return newptr;
        }else return nullptr;
    }

private:
    UserBufferArena(const UserBufferArena&) = delete;
    UserBufferArena& operator=(const UserBufferArena&) = delete;

    inline char* currentPointer(){ return (char*)currentChunk + currentSize; }

    struct chunk{
        chunk* next = 0;
    };

    static const ub8 kChunkSize = ALIGN(sizeof(chunk)); 
    
    ub4         currentSize = 0; 
    // currentChunk指向user buffer时，前16位存其大小，顺便用于标识目前正在
    // 使用user buffer；当它指向Arena构造的chunk时，行为与Arena中一致。
    chunk*      currentChunk = 0; 
    ub4         chunkCapacity; 
};


}
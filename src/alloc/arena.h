#pragma once

#include "common.h"

namespace wjp{

class Arena{
public:
    static const int kSmallShift = 3;
    
    Arena(ub4 chunkCapacity = 4*kPageSize): chunkCapacity(chunkCapacity){}

    ~Arena(){
        while (currentChunk){
            auto tofree=currentChunk;
            currentChunk = currentChunk->next;
            std::free(tofree);
        }
    }

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

}
#pragma once

#include "common.h"

namespace wjp{

class Arena{
public:
    static const bool kNeedFree = false;
    Arena(void* userBuffer=0, ub4 userBufferSize=0, ub4 chunkCapacity=4*kPageSize):
                chunkList(0), chunkCapacity(chunkCapacity), userBuffer(userBuffer)
    {

    }


private:
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    bool appendChunk(ub4 chunkCapacity){
        void* new_chunk;
        if(posix_memalign( &new_chunk, 64, ALIGN(sizeof(chunk))+chunkCapacity)==0) {
            

        }else return false;
    } 

    struct chunk{ 
        chunk* next = 0;
    };
    chunk*      chunkList;
    ub4         chunkCapacity;
    void*       userBuffer;
};

}
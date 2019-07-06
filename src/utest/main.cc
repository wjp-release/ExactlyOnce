#include "common.h"

#include "alloc/arena.h"

using namespace wjp;

int main(){
    UserBufferArena arena;
    arena.alloc(1000);

    return 0;
}
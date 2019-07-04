#include "common.h"

#include "alloc/arena.h"

using namespace wjp;

int main(){
    Arena arena;
    arena.alloc(1000);

    return 0;
}
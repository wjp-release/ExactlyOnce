#pragma once

#include "common.h"

namespace wjp{

struct BuddyBlock{
public: 
    ub4     next    : 20; // next block's offset in pages
    ub1     magic1  : 4;
    ub1     order   : 8; // lg2(blocksize) = order, at most 10
    ub4     prev    : 20; // prev block's offset in pages
    ub1     magic2  : 4; 
    ub1     free    : 1; // free, unused, in list
    ub1     first   : 1; // first node in list
    ub1     last    : 1; // last node in list
    ub1     left    : 1; // left buddy
    ub1     magic3  : 4; 
    static const ub1 kMagicNumber1 = 0x0a;
    static const ub1 kMagicNumber2 = 0x0b;
    static const ub1 kMagicNumber3 = 0x0c;

    void init(ub1 order, char* startaddr){
        this->order = order;
        this->free = 1;
        this->first = 0;
        this->last = 0;
        this->left = isLeftBuddy(startaddr) ? 1 : 0;
        this->magic1 = kMagicNumber1;
        this->magic2 = kMagicNumber2;
        this->magic3 = kMagicNumber3;
    }

    inline ub4 blockSize(){ return (1 << order) << kPageSizeOrder; }

    inline char* userAddress(){ return (char*)this + 8; }

    inline ub4 offset(char* startaddr){ return (ub4)((char*)this - startaddr);}

    inline ub4 offsetInPages(char* startaddr){ return offset(startaddr) >> kPageSizeOrder; }

    inline ub4 offsetInBlockNumber(char* startaddr){ return offsetInPages(startaddr) >> order; }

    BuddyBlock* prevBlock(char* startaddr){
        if (first) return nullptr;
        return (BuddyBlock*)(startaddr + (prev << kPageSizeOrder));
    }

    BuddyBlock* nextBlock(char* startaddr){
        if (last) return nullptr;
        return (BuddyBlock*)(startaddr + (next << kPageSizeOrder));
    }

    BuddyBlock* myBuddy(char* startaddr){
        BuddyBlock* buddy;
        if (left) buddy = (BuddyBlock*) ((char*)this + blockSize());
        else buddy = (BuddyBlock*) ((char*)this - blockSize());
        return buddy;
    }

    BuddyBlock* split(char* startaddr){
        assert(free == 0);
        order--;
        left = isLeftBuddy(startaddr) ? 1 : 0;
        BuddyBlock* secondhalf = (BuddyBlock*)((char*)this + blockSize());
        secondhalf->init(order, startaddr);
        return secondhalf;
    }

    // merge with right buddy
    void grow(char* startaddr){
        assert(free == 0);
        order++;
        left = isLeftBuddy(startaddr) ? 1 : 0;
    }
private:
    inline bool isLeftBuddy(char* startaddr){
        return (offsetInBlockNumber(startaddr) & 0x01) == 0x00;
    }
};


class BuddySystem{
public:
    static const int kMaxOrder = 10;
    static const int kMaxPagesPerBlock = (1 << kMaxOrder);

    BuddySystem(ub4 maxpages){
        startaddr = (char*) malloc64(maxpages << kPageSizeOrder);
        if (!startaddr) throw std::runtime_error("malloc64 error");
        ub4 maxorder = kMaxOrder;
        while (!(maxpages >> maxorder)) maxorder--;
        bank.assign(maxorder + 1, nullptr); // 0, 1, ... maxorder
        for (int curorder = maxorder, pages = maxpages; curorder >= 0; curorder--){
            ub4 nrblocks = pages >> curorder;
            if (nrblocks == 0) continue;
            char* addr = startaddr + ((maxpages - pages) << kPageSizeOrder);
            for (ub4 i = 0; i < nrblocks; i++, addr += (1 << curorder) << kPageSizeOrder){
                emplace(addr, (ub1)curorder);
            }
            pages -= nrblocks << curorder;
        }

    }

    char* alloc(ub4 size){
        size += 8; // 8 bytes for meta data
        ub4 pages = size >> kPageSizeOrder;
        if (size > (pages << kPageSizeOrder)) pages++;
        if (pages > kMaxPagesPerBlock) return nullptr;
        auto minorder = decideOrder(pages);
        for (int order = minorder; order <= kMaxOrder; order++){
            auto block = pop((ub1)order);
            if (block){
                if (order > minorder) shrink(block, minorder);
                return block->userAddress();
            }
        }
        return nullptr;
    }

    void free(char* ptr){
        BuddyBlock* block = (BuddyBlock*)(ptr - 8);
        while (block->order < kMaxOrder){
            auto buddy = getBuddy(block);
            if (buddy) block = merge(block, buddy);
            else break;
        }
        push(block);
    }
private:
    // 合并互为buddy的A、B，返回合并后的块
    BuddyBlock* merge(BuddyBlock* A, BuddyBlock* B){
        assert(A->order == B->order && A->order < kMaxOrder && A->left != B->left);
        if (B->left) A = B;
        A->grow(startaddr);
        return A;
    }

    void shrink(BuddyBlock* block, ub1 targetOrder){
        while (block->order > targetOrder){
            auto second = block->split(startaddr);
            push(second);
        }
    }

    BuddyBlock* pop(ub1 order){
        auto block = bank[order];
        if (!block) return nullptr;
        erase(block);
        return block;
    }

    void emplace(char* addr, ub1 order){
        BuddyBlock* block = (BuddyBlock*)addr;
        block->init(order, startaddr);
        push(block);
    }

    void push(BuddyBlock* block){
        auto first = bank[block->order];
        bank[block->order] = block;
        block->first = 1;
        block->free = 1;
        if (!first){
            block->last = 1;
        }else{
            block->next = first->offsetInPages(startaddr);
            first->prev = block->offsetInPages(startaddr);
        }
    }

    void erase(BuddyBlock* block){
        if (!block) return;
        block->free = 0;
        if (block->first && block->last){
            bank[block->order] = nullptr;
        }else if(block->first){
            bank[block->order] = block->nextBlock(startaddr);
            bank[block->order]->first = 1;
        }else if(block->last){
            auto prev = block->prevBlock(startaddr);
            prev->last = 1;
        }else{
            auto prev = block->prevBlock(startaddr);
            auto next = block->nextBlock(startaddr);
            prev->next = next->offsetInPages(startaddr);
            next->prev = prev->offsetInPages(startaddr);
        }
    }

    BuddyBlock* getBuddy(BuddyBlock* me){
        BuddyBlock* buddy = me->myBuddy();
        if (buddy->order == me->order && buddy->free){
            erase(buddy);
            return buddy;
        }else{
            return nullptr;
        }
    }

    ub1 decideOrder(ub4 pages){
        for (ub1 i = 0; i <= kMaxOrder; i++){
            if (pages <= (1u << i)) return i;
        }
        return 0; // disable warning, unreachable 
    }

    std::vector<BuddyBlock*> bank;
    char* startaddr;
};



}
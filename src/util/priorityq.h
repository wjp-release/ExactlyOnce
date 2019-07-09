#pragma once

#include "common.h"

namespace wjp{

template < typename LessPredicate, typename ValueType >
class BinaryHeap{
public:
    BinaryHeap(LessPredicate lessPredicate = LessPredicate{}) : lessPredicate(lessPredicate){}

    BinaryHeap(std::vector<ValueType>&& x, LessPredicate lessPredicate = LessPredicate{}) : lessPredicate(lessPredicate) {
        arr.swap(x);
        makeHeap();
    }

    BinaryHeap(const std::vector<ValueType>& x, LessPredicate lessPredicate = LessPredicate{}) : lessPredicate(lessPredicate), arr(x) {
        makeHeap();
    }

    BinaryHeap(const BinaryHeap& rhs) : lessPredicate(rhs.lessPredicate), arr(rhs.arr) {}

    BinaryHeap(BinaryHeap&& rhs) : lessPredicate(rhs.lessPredicate) {
        arr.swap(rhs.arr);
    }

    void heapify(int index = 0){
        int root = index;
        if( left(index) >= arr.size()) return;

    }

    void push(){


    }

    ValueType pop(){

    }

    void update(int index, ValueType newval){

    }

private:
    static inline int left(int i) { return (i << 1) + 1; }
    static inline int right(int i) { return (i << 1) + 2; }
    static inline int parent(int i) { return (i - 1) >> 1; }

    LessPredicate lessPredicate;
    std::vector<ValueType> arr;
};



}
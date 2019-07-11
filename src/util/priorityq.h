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
        if (left(index) >= arr.size()) return;
        if (less(left(index), root)) root = left(index);
        if (less(right(index), root)) root = right(index);
        if (root != index){
            swaparr(index, root);
            heapify(root);
        }
    }

    void push(const ValueType& value){
        arr.push_back(value);        
        insert(arr.size()-1, 0, value);
    }

    ValueType pop(){
        if (arr.size() == 0) throw std::runtime_error("heap empty");
        auto duh = arr[0];
        arr[0] = arr.back();
        arr.pop_back();
        heapify(0);
        return duh;
    }

    void update(int index, const ValueType& newval){
        auto oldval = arr[index];
        arr[index] = newval;
        if (lessPredicate(newval, oldval)){
            heapify(index);
        }else{
            insert(index, 0, newval);
        }
    }

    void makeHeap(){
        if (arr.size() < 2) return;
        if (arr.size() == 2){
            if (less(1,0)) swaparr(0, 1);
            return;
        }
        size_t last = arr.size() - 1;
        for (int p = parent(last); p >= 0; p--) heapify(p);
    }

    bool isHeapUntil(){
        size_t p = 0;
        size_t n = arr.size();
        for (size_t child = 1; child < n; ++child){
            if (less(child, p)) return child;
            if ((child & 1) == 0) ++p;
        }
        return n;
    }

    inline size_t size() { return arr.size(); }

    inline bool empty() { return arr.empty(); }

private:
    static inline int left(int i) { return (i << 1) + 1; }
    static inline int right(int i) { return (i << 1) + 2; }
    static inline int parent(int i) { return (i - 1) >> 1; }

    inline bool less(size_t id1, size_t id2){
        return lessPredicate(arr[id1], arr[id2]);
    }

    inline bool equal(size_t id1, size_t id2){
        return !less(id1, id2) && !less(id2, id1);
    }

    inline void swaparr(size_t a, size_t b){
        std::swap(arr[a], arr[b]);
    }

    LessPredicate lessPredicate;
    std::vector<ValueType> arr;
};



}
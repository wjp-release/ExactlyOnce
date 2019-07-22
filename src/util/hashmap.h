#pragma once

#include "common.h"
#include "siphash.h"

namespace wjp{

struct SipHash{
    ub8 operator()(const ub1* in, const ub4 len){
        return siphash(in, len, (const ub1*)"1234567812345678");
    }
};

template < typename K, typename V, typename Less = std::less<K>, typename Hash = SipHash, int InitialOrder = 2 >
class Hashmap{
public:
    static const int kInitalOrder = InitialOrder;

    struct Entry{
        K key;
        V value;
        Entry* next;
    };

    struct Table{
        void initBuckets(){
            ub4 cap = capacity();
            buckets = (Entry**) malloc(sizeof(Entry*) * cap);
            for (ub4 i = 0; i < cap; i++) buckets[i] = nullptr;
        }

        Table(){}
        
        ~Table(){
            if (buckets) free(buckets);
        }
        
        bool bucketsInited(){
            return buckets != nullptr;
        }
        
        void reset(){
            if (buckets) free(buckets);
            buckets = nullptr;
            used = 0;
        }
        
        Entry* removeAt(int id, const K& key, Hashmap* hmap){
            //todo
            if (!bucketsInited()) return nullptr;
            Entry* entry = buckets[id];
            Entry* prev = nullptr;
            while (entry){
                if (key == entry->key || hmap->equal(key, entry->key)){
                    // now we find the entry, unlink it
                    if (prev) prev->next = entry->next;
                    else buckets[id] = entry->next;
                    return entry;
                }
                entry = entry->next;
                prev = entry;
            }
            return nullptr;
        }

        Entry* searchAt(int id, const K& key, Hashmap* hmap){
            if (!bucketsInited()) return nullptr;
            Entry* entry = buckets[id];
            while (entry){
                if (key == entry->key || hmap->equal(key, entry->key)) return entry;
                entry = entry->next;
            }
            return entry;
        }

        Entry* createNewEntryAt(int id){
            Entry* newent = (Entry*) malloc(sizeof(Entry));
            memset(newent, 0, sizeof(Entry));
            add(newent, id);
            return newent;
        }

        void add(Entry* newent, int id){
            used++;
            newent->next = buckets[id];
            buckets[id] = newent;
        }
        
        ub4 capacity(){
            return 1 << order;
        }
        
        ub4 mask(){
            return capacity() - 1;
        }

        Entry** buckets = nullptr;
        ub4 used = 0;
        ub1 order = kInitalOrder;
    };

    struct Iterator{
        Iterator(Hashmap* hmap) : hmap(hmap){
            hmap->nriters++;
        }

        ~Iterator(){
            hmap->nriters--;
        }

        Entry& operator*() const {
            return *cur;
        }

        Entry* operator->() const {
            return cur;
        }

        Iterator& operator++(){
            next();
            return *this;
        }

        Iterator operator++(int){
            auto tmp = *this;
            next();
            return tmp;
        }
    private:
        Entry* currentBucketBegins(){
            if (index == -1) return nullptr;
            return hmap->tables[future].buckets[index];
        }
        
        void next(){
            if (cur && cur->next){
                cur = cur->next;
                return;
            }
            if ((ub4)index < hmap->tables[future].capacity()){
                index++;
            }else if(future == 0 && hmap->isRehashing()){
                future = 1;
                index = 0;
            }else{
                future = 0;
                index = -1;
            }
            cur = currentBucketBegins();
        }

        Hashmap* hmap;
        Entry* cur = nullptr;
        int index = -1
        int future = 0; // 0 or 1
    };

    Hashmap(){
        tables[0].initBuckets(); // rehash table remains uninited
    }

    Iterator begin(){
        Iterator iter(this);
        iter++;
        return iter;
    }

    Iterator end(){
        return Iterator(this);
    }

    bool empty(){
        return size() == 0;
    }

    ub4 size(){
        return tables[0].used + tables[1].used;
    }

    ub4 nrbuckets(){ 
        return tables[0].capacity() + tables[0].capacity();
    }

    Entry* erase(const K& key){
        rehashOnEveryOperation();
        ub8 hash = hasher((const ub1*)&key, sizeof(K));
        ub4 index = tables[0].mask() & hash;
        Entry* entry = tables[0].removeAt(index, key, this);
        if (entry) return entry;
        return nullptr;
    }
    

private:
    ub4 nriters = 0;
};



}
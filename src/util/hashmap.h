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
        tables[0].initBuckets(); // rehash table must remain uninited
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

    bool exists(const K& key){
        return !find(key);
    }

    Entry* find(const K& key){
        if (empty()) return nullptr;
        rehashOnEveryOperation();
        ub8 hash = hasher((const ub1*)&key, sizeof(K));
        auto entry =  tables[0].searchAt(tables[0].mask() & hash, key, this);
        if (entry) return entry;
        if (!isRehashing()) return nullptr;
        return tables[1].searchAt(tables[1].mask() & hash, key, this);
    }

    // Find key, but if it does not exist, place and return a new entry,
    // which has its key set. Caller should set its value immediately.
    Entry* findOrCreateNew(const K& key, bool* existing = nullptr){
        rehashOnEveryOperation();
        if (existing) *existing = true;
        ub8 hash = hasher((const ub1*)&key, sizeof(K));
        ub4 index = tables[0].mask() & hash;
        Entry* entry = tables[0].searchAt(index, key, this);
        if (entry) return entry;
        if (isRehashing()){
            index = tables[1].mask() & hash;
            Entry* entry = tables[1].searchAt(index, key, this);
            if (entry) return entry;
        }
        // search failed, now we insert new entry
        if (existing) *existing = false;
        Table& which = isRehashing() ? tables[1] : tables[0];
        auto newEntry =  which.createNewEntryAt(index);
        newEntry->key = key;
        return newEntry;
    }

    V& operator[](const K& key){
        Entry* entry = findOrCreateNew(key);
        assert(entry);
        return entry->value;
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
        if (isRehashing()){
            index = tables[1].mask() & hash;
            Entry* entry = tables[1].removeAt(index, key, this);
            if (entry) return entry;
        }
        return nullptr; // key not found
    }
    
private:
    static const int kRehashRatio = 100;

    void rehash(int n = 1){
        if (nriters || isRehashing()) return false;
        int empty_visits = n << 5; // at most 32 empty visits
        for (; n != 0 && tables[0].used != 0; n--){
            assert(tables[0].capacity() > (ub4)rehashid);
            while (tables[0].buckets[rehashid] == nullptr){
                rehashid++;
                if (--empty_visits == 0) return true;
            } // now we find an non-empty bucket
            auto entry = tables[0].buckets[rehashid];
            while (entry){
                auto next = entry->next;
                ub8 hash = hasher((const ub1*)&key, sizeof(K));
                ub4 hashid = hash & tables[1].mask();
                tables[1].add(entry, hashid);
                tables[0].used--;
                entry = next;
            }
            tables[0].buckets[rehashid] = nullptr;
            rehashid++;
        }
        if (tables[0].used == 0){ // rehash finished
            free(tables[0].buckets);
            tables[0] = tables[1];
            tables[1].reset();
            rehashid = -1;
        }
        return true;
    }

    void rehashOnEveryOperation(){
        if (isRehashing()) rehash();
        else{
            if (needsRehashing()){
                startsRehashing();
                rehash(); // rehash once on initial rehash
            }
        }
    }

    bool needsRehashing(){
        return tables[0].used >= kRehashRatio / 100 * tables[0].capacity();
    }

    bool startsRehashing(){
        assert(tables[0].bucketsInited());
        if (isRehashing()) return false;
        tables[1].order = tables[0].order + 1;
        tables[1].initBuckets();
        rehashid = 0;
        return true;
    }

    bool isRehashing(){
        return rehashid != -1;
    }

    bool equal(const K& k1, const K& k2){
        return (!lesspred(k1, k2)) && (!lesspred(k2, k1));
    }

    Less lesspred;
    Hash hasher;
    Table tables[2];
    int rehashid = -1; // next id in tables[0].buckets to rehash
    ub4 nriters = 0;
};



}
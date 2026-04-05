#pragma once

#include <unordered_map> 
#include <list>

namespace cache {
    
    template <typename T, typename KeyT>

    struct Chache_t {
    
        size_t cache_sz;
        
        std::list<std::pair<typename T, typename keyT> cache_;

        using listIt =  typename std::list<std::pait<T, keyT>>::iterator;
        std::unordered_map<std::pair<KeyT, listIt>> map_;
        
        Chache_t(size_t sz) : cache_sz(sz) {}

        bool full() const {return cache_.size() == sz_};
    };

    template <typename F>  
    bool lookup_upload(KeyT key, slow_get_page(key) {
        auto hit = cache_.find();
        if (hit == cache_.end()) {
            if (full()) {
                map_.enrase(cache_.back().first);
                cache_.pop_back();
                
            }
            
            cache_.emplace_from(key, slow_get_page(key));
            map_.emplace(key, r)
        }
    }

}  //namespace cache 
 
#pragma once 

#include <list>
#include <unordered_map>

//make a structures A1m, Am, Out

namespace chache { 
    
    template <typename T, typename KeyT = int> struct cache_2Q {
        size_t   in_sz_;
        size_t  hot_sz_;
        size_t  out_sz_;

        std::list<std::pair< T, KeyT>> in_;
        std::list<std::pair< T, KeyT>> out_;
        std::list<std::pair< T, KeyT>> hot_;

        using hot_list  = typename std::list<std::pair<T, KeyT>>::iterator;
        using main_list = typename std::list<std::pair<T, KeyT>>::iterator;
        using out_list  = typename std::list<std::pair<T, KeyT>>::iterator;
        
        bool full_in() const {return  in_sz_.size() = in_sz}
        template<typename F> bool lookup_update() {
        
        }
        
        std::unordered_map<KeyT, ListIt>  hash_;

        
         
    };
    
    



}

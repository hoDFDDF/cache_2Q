#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>


namespace cache {

    template <typename keyT, typename Value>

    class cache_LFU_ {
        size_t capasity_;
        size_t min_freq_;

        struct FrequencyGroup {
            std::size_t frequency = 0;
            std::list<keyT> keys;
        };
        

        std::unordered_map<keyT, Value> entries;
        std::unordered_map<Freq
        bool get(keyT key) {

        }
 
        bool put(keyT key, Value value) {

        }        

        void touch()



        public:
        
            explicit cache_LFU_(std::size_t capasity) : capasity_{capacity} {
        
            template <typename F> 

            bool lookUpUpdate(keyT key, F slow_get_page) {
                
            }


        }
    }





}
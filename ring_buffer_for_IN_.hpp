#pragma once

#include <unordered_map>
#include <vector>

template <typename keyT, typename Value>
    struct ring_buffer{
        size_t in_sz_;
        std::vector<std::pair<keyT, Value>> cache_in_;
        size_t capacity;
        
         //конструктор
        int tail = 0;
        int head = 0;
         
        bool FullRing()  const { return capacity ==  in_sz_; }
        bool EmptyRing() const {       return capacity == 0; }
        

            auto RingPush(keyT key){
    
                if (FullRing()) { RingPop(); }
                cache_in_[head].first = key;
                
                head = (head + 1) % in_sz_;

                capacity++;

                return cache_in_[head];
            }
            
            keyT RingPop() {
                if (!EmptyRing()) { 

                    keyT temp_tail_key = cache_in_[tail].first; 
                 
                    cache_in_.erase(cache_in_.begin());

                    tail = (tail + 1) % in_sz_;
                    capacity--;

                    return temp_tail_key;
                }
                return keyT{};                
            }
         
    };


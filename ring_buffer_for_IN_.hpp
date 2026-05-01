#pragma once

#include <unordered_map>
#include <vector>

template <typename keyT, typename Value>
    struct ring_buffer{
        size_t in_sz_;
        std::vector<std::pair<keyT, Value>> cache_in_;
        size_t capacity = 0;
        
        int tail = 0;
        int head = 0;

        ring_buffer(size_t size = 0) : in_sz_(size) {
            if (in_sz_ > 0) {   
                cache_in_.resize(in_sz_);
            } 
        }
         
        bool FullRing()  const { return capacity ==  in_sz_; }
        bool EmptyRing() const {       return capacity == 0; }
        
        auto RingPush(keyT key){
            int temp_head = head;

            cache_in_[temp_head].first = key;
            
            head = (head + 1) % in_sz_;
            capacity++;

            return cache_in_[temp_head];
        }
            
        keyT RingPop() {
            if (!EmptyRing()) { 
                keyT temp_tail_key = cache_in_[tail].first; 

                tail = (tail + 1) % in_sz_;
                capacity--;
                return temp_tail_key;
            }
            return keyT{};                
        }
         
    };
#pragma once 

#include <list>
#include <unordered_map>
#include <cmath>
#include <vector>
#include <cassert>

#include "ring_buffer_for_IN_.hpp"

const double hot_percent_of_cahce_sz_ = 0.75;
const double out_percent_of_cahce_sz_ = 0.25;

namespace cache {

    template <typename keyT, typename Value>

        struct cache_2Q_ {

            size_t cache_sz_;
            size_t   hot_sz_;
            size_t   out_sz_;

            std::list<std::pair<keyT, Value>> cache_hot_;
            ring_buffer<keyT, Value> in_buffer;
            std::list<keyT> cache_out_;

            cache_2Q_(size_t cache_sz_) : cache_sz_(cache_sz_), in_buffer(cache_sz_ - static_cast<size_t>(std::ceil(hot_percent_of_cahce_sz_ * cache_sz_))) { 
                hot_sz_  = static_cast<size_t>(std::ceil(hot_percent_of_cahce_sz_ * cache_sz_));
                out_sz_ = static_cast<size_t>(std::floor(out_percent_of_cahce_sz_ * cache_sz_));
            }

            bool FullHot () const { return  cache_hot_.size() == hot_sz_; }
            bool FullOut () const { return  cache_out_.size() == out_sz_; } 

            enum class listIt{ IN_, HOT_, OUT_ };
            
            struct node {
                Value val;
                listIt state;

                typename std::list<std::pair<keyT, Value>>::iterator it;  

                //TODO Add check, so that it small input capacity, the size of IN_ wont't be zero
                node (Value v, listIt s, typename std::list<std::pair<keyT, Value>>::iterator i = {})
                    :val(v), state(s), it(i) {} 
            };

            std::unordered_map<keyT, node> hash_;

            template <typename F>

                bool LookUpUpdate(keyT key, F slow_get_page) {
            
                    auto hit_it = hash_.find(key);
                    
                    if (hit_it != hash_.end()) {

                        if (hit_it->second.state == listIt::IN_) { 
                            return true;
                        }

                        else if (hit_it->second.state == listIt::HOT_) {

                            if (FullHot()) {

                                cache_out_.emplace_front(key);

                                hash_.erase(cache_hot_.back().first);
                                cache_hot_.pop_back();
                            }

                            auto hot_eltit = hit_it->second;
                            cache_hot_.splice(cache_hot_.begin(), cache_hot_, hot_eltit.it);

                            return true;
                        }
                         
                        else if (hit_it->second.state == listIt::OUT_) {
                            if (FullOut()) {
                                hash_.erase(cache_out_.back());
                                cache_out_.pop_back();            
                            }

                            cache_hot_.emplace_front(key, slow_get_page(key));

                            hit_it->second.state  =  listIt::HOT_;
                            hit_it->second.it = cache_hot_.begin();

                            return false;
                        }
                    }

                    if (in_buffer.FullRing()) {
                        keyT received_key = in_buffer.RingPop(); 
                        cache_out_.emplace_front(received_key);
                        hash_.at(received_key).state = listIt::OUT_;                         
                    }

                    Value fresh_data = slow_get_page(key);
                    in_buffer.RingPush(key);

                    hash_.emplace(key, node {fresh_data, listIt::IN_});

                    return false;

                }
        };
};


           
            
#pragma once 

#include <list>
#include <unordered_map>
#include <cmath>
#include <vector>
#include <cassert>

#include "ring_buffer_for_IN_.hpp"

const double hot_percent_of_cahce_sz_ = 0.75;
const double in_percent_of_cahce_sz_ = 0.25;

namespace cache {

    template <typename keyT, typename Value>

        struct cahche_2Q_ {

            size_t cache_sz_;
            size_t in_sz_;
            size_t hot_sz_;
            size_t out_st_;

            std::list<std::pair<keyT, Value>> cache_hot_;

            //std::vector<std::pair<keyT, Value>>> cache_in_;
        
            std::list<keyT> cache_out_;

            cache_2Q_(size_t cache_sz_) : cache_sz_(cache_sz_){
                hot_sz_  = static_cast<size_t>(std::ceil(hot_percent_of_cahce_sz_ * cache_sz_));
                in_sz_ = static_cast<size_t>(std::floor(in_percent_of_cahce_sz_   * cache_sz_));
                //add out

            }

            bool FullHot () const { return  cache_hot_.size() == hot_sz_; }
            bool FullIn  () const { return  cache_in_.size()  ==  in_sz_; }
            bool FullOut () const { return  cache_out_.size() =  out_sz_; } 
  
            Value AddFromINtoOut() {

            }
            enum class listIt{ IN_, HOT_, OUT_ };
            
            struct node {
                Value val;
                listIt state;
                
                typename std::list<keyT>::interator it;  
            }

            std::unordered_map<keyT, node> hash_;
            template <typename keyT, typename F>

                bool LoopUpdateIn(keyT key, F slow_get_page) {
                
                    auto hit_it = hash_.find(keyT);
                    
                    if (hit_it != hash_.end()) {//чекаем, вообще знаем ли мы такой ключ и находится ли он в данный момент в одной из очередей

                        if (hit_it->second.state == listIt::IN_) {
                            
                            assert(hit_it->second.state == listIt::IN_ && "Expected in_ queue paramets (vector pair)");

                            hash_.fro


                        }

                        else if (hit_it->second.state == listIt::HOT_) {

                            
                            if (FullIn()) {
                                hash_in_.erase(cache_in_.back().first);
                                cache_in_.pop_back();
                            }
                        
                            cache_in_.emplace_front(key, slow_get_page(key));
                            hash_.emplace(key, cache_in_.begin());

                            return false;

                            auto in_eltit = hit_in_flag->second;
                            cache_in_.splice(cache_in_.begin(), cache_in_, in_eltit);
                            return true;

                        }
                         
                        else if (hit_it->second.state == listIt::OUT_) {
                            
                            //вызываем нужную функцию 
                        }
                        
                        else {
                            RingPush(keyT); 
                        }

                        //key in noone queue, below I add it to IN_queue: like carantin;
                        

                        


                    }

                       
                }
        };
};


           
            
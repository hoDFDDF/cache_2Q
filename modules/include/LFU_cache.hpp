#pragma once

#include <cstddef>
#include <iostream>
#include <iterator>
#include <vector>
#include <unordered_map>
#include <list>

namespace cache {

    template <typename keyT, typename Value>

    class cache_LFU_ {
        std::size_t capacity_;
        
        struct FrequencyGroup {
            std::size_t frequency = 0;
            std::list<keyT> keys;
        };

        std::list<FrequencyGroup> Fgroups_;
        
        struct Entry {
            Value value;
            typename std::list<FrequencyGroup>::iterator group_it;
            typename std::list<keyT>::iterator key_it;
        };

        std::unordered_map<keyT, Entry> entries_;

        // is there element in cache or not?
        bool get(keyT key) {
            if (entries_.contains(key)) return true;

            return false;
        }
 
        // add new element to cache
        void put(keyT key, Value value) {
            if (Fgroups_.empty() || Fgroups_.front().frequency != 1) {
                Fgroups_.push_front(FrequencyGroup{1, {}});
            }
                auto group_it = Fgroups_.begin();
                group_it->keys.push_front(key);

                auto key_it = group_it->keys.begin();

                entries_.emplace(key, Entry{value, group_it, key_it});
        }        

        // increase frequency existing element
        void touch(Entry& entry) {
            auto cur_group_it  = entry.group_it;
            auto next_group_it = std::next(cur_group_it);
            std::size_t new_frequency = entry.group_it->frequency + 1;

            if (next_group_it == Fgroups_.end() ||
                next_group_it->frequency != new_frequency) {

                    next_group_it = Fgroups_.insert(next_group_it,
                                                   FrequencyGroup{new_frequency, {}});
            }
            
            next_group_it->keys.splice(next_group_it->keys.begin(),
                                       cur_group_it->keys, entry.key_it);

            entry.group_it = next_group_it;

            if (cur_group_it->keys.empty()) {
                Fgroups_.erase(cur_group_it);
            }
        }

        // erase one element (least frequency and least used)
        void evictOne() {
            auto first_group_it  = Fgroups_.begin();

            entries_.erase(first_group_it->keys.back());
            first_group_it->keys.pop_back();

            if (first_group_it->keys.empty()) {
                Fgroups_.erase(first_group_it);
            }
        }


        public:
        
            // constructor
            explicit cache_LFU_(std::size_t capacity)
                : capacity_{capacity} {}

            template <typename F> 

            // the main public function. True if element is in cache, false against
            bool lookUpUpdate(const keyT& key, F slow_get_page) {
                if (capacity_ == 0) return false;

                if (get(key)) {
                    auto entry_it = entries_.find(key);
                    touch(entry_it->second);
                    return true;
                }
                
                if (entries_.size() == capacity_)
                    evictOne();
                put(key, slow_get_page(key));
               
                return false;
            }


    };





}
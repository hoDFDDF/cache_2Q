#include <iostream>
#include <cassert>

#include "LFU_cache.hpp"




int slow_get_page(int key) { return key; }

int main () {
    int hits = 0; 
    size_t cache_size     = 0;
    size_t cache_capacity = 0;
    
    assert(std::cin.good());

    std::cin >> cache_size >> cache_capacity;
    cache::cache_LFU_<int, int> cache_1{cache_capacity};

    for (int i = 0; i < cache_size; i++) {
        int key;

        assert(std::cin.good());
        std::cin >> key;
        if (cache_1.lookUpUpdate(key, slow_get_page)) {
            hits++;
        }
    }
    std::cout << hits;

    return 0;
}
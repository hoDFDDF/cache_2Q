#include <iostream>
#include <cassert>

#include "2Q_cache.hpp"

int slow_get_page(int key) { return key; }

int main(){
    int hits = 0; 
    size_t sz;
    size_t el;
    
    assert(std::cin.good());

    std::cin >> sz >> el;
    cache::cache_2Q_<int, int> c{el};

    for (int i = 0; i < sz; i++) {
        int key;

        assert(std::cin.good());
        std::cin >> key;
        if (c.LookUpUpdate(key, slow_get_page)) {
            hits++;
        }
    }
    std::cout << hits;
    return 0;
}
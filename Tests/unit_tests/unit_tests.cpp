#include "2Q_cache.hpp"
#include "ring_buffer_for_IN_.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>

template <typename keyT> keyT slow_get_page(keyT key) {
    return key;
}

// ring tests  
TEST (RingBufferTests, AddElemToRingBuffer){
    size_t in_sz_ = 5;
    ring_buffer<int, int> in_buffer{in_sz_};

    int received_key_01  = in_buffer.RingPush(10).first; 
    int received_key_02 = in_buffer.RingPush(20).first;

    EXPECT_EQ(in_buffer.cache_in_[in_buffer.head].first, 20);
    EXPECT_EQ(in_buffer.cache_in_[in_buffer.tail].first, 10);
}

TEST (RingBufferTests, isFullBuffer){
    size_t in_sz_ = 3;
    ring_buffer<int, int> in_buffer{in_sz_};

    int received_key_01  = in_buffer.RingPush(1).first; 
    int received_key_02 = in_buffer.RingPush(5).first;
    int received_key_03 = in_buffer.RingPush(2).first;
    
    EXPECT_THAT(in_buffer.cache_in_, testing::ElementsAre(testing::Pair(1, testing::_),
                                                          testing::Pair(2, testing::_),
                                                          testing::Pair(5, testing::_)));
    EXPECT_TRUE(in_buffer.isFull());
}

TEST (RingBufferTests, WasElemAddedToOut){
    size_t cache_sz = 4;
    cache::cache_2Q_<int, int> cache{cache_sz};

    cache.LookUpUpdate(1, slow_get_page<int>);
    cache.LookUpUpdate(2, slow_get_page<int>);
    
    EXPECT_EQ(cache.GetKeyStatus(1),  (cache::cache_2Q_<int, int>::listIt::OUT_));
    EXPECT_THAT(cache.cache_out_, testing::ElementsAre(1));
}

// check allocation memory in constructor of 2Q_cache
TEST (CacheConstructorCheck, SizeDestibutionBetweenQueues) {
    size_t cache_sz_ = 100;
    cache::cache_2Q_<int, int> cache{cache_sz_};
 
    EXPECT_EQ(cache.in_sz_,  25); 
    EXPECT_EQ(cache.hot_sz_, 75);
    EXPECT_EQ(cache.out_sz_, 50);
}

TEST (CacheConsructorCheck, ExtreamlySmallSizeDistibution){
    size_t cache_sz_ = 1;
    cache::cache_2Q_<int, int> cache{cache_sz_};
    
    EXPECT_TRUE(cache.in_sz_  != 0);
    EXPECT_TRUE(cache.in_sz_  != 0);
    EXPECT_TRUE(cache.out_sz_ != 0);
} 

// check wokr of individual caches
TEST (HotCacheCheck, SpliceMethodWork){
    size_t cache_sz_ = 4;
    cache::cache_2Q_<int, int> cache{cache_sz_};
    
    cache.LookUpUpdate(2, slow_get_page<int>); 
    cache.LookUpUpdate(1, slow_get_page<int>); 
    cache.LookUpUpdate(3, slow_get_page<int>); 
    cache.LookUpUpdate(4, slow_get_page<int>); 

    cache.LookUpUpdate(3, slow_get_page<int>); 
    EXPECT_EQ(cache.GetKeyStatus(3),  (cache::cache_2Q_<int, int>::listIt::HOT_));
    EXPECT_EQ(cache.cache_hot_.front().first, 3);

    cache.LookUpUpdate(1, slow_get_page(1));
    EXPECT_EQ(cache.GetKeyStatus(1), (cache::cache_2Q_<int, int>::listIt::HOT_));
    EXPECT_EQ(cache.cache_hot_.front().first, 1);
}   

TEST (HotCacheCheck, isFullHot){   
    size_t cache_sz_ = 2;
    cache::cache_2Q_<int, int> cache{cache_sz_};
    
    cache.LookUpUpdate(1, slow_get_page<int>);
    cache.LookUpUpdate(2, slow_get_page<int>);
    cache.LookUpUpdate(1, slow_get_page<int>);

    EXPECT_TRUE(cache.FullHot());
}

TEST (HotCacheCheck, DropToOut){   
    size_t cache_sz_ = 2;
    cache::cache_2Q_<int, int> cache{cache_sz_};
    
    cache.LookUpUpdate(1, slow_get_page<int>);
    cache.LookUpUpdate(2, slow_get_page<int>);
    cache.LookUpUpdate(1, slow_get_page<int>);
    EXPECT_EQ(cache.GetKeyStatus(1), (cache::cache_2Q_<int, int>::listIt::HOT_));

    cache.LookUpUpdate(2, slow_get_page<int>);
    cache.LookUpUpdate(3, slow_get_page<int>);
    cache.LookUpUpdate(2, slow_get_page<int>);

    EXPECT_EQ(cache.GetKeyStatus(2), (cache::cache_2Q_<int, int>::listIt::OUT_));
    EXPECT_TRUE(cache.hash_.find(1) == cache.hash_.end());
}


//check hash table
TEST (HashTableCheck,  NoMemoryLeaksUnderStress) {
    size_t  cache_sz_ = 10;
    size_t request_num = 1000;
    cache::cache_2Q_<int, int> cache{cache_sz_};

    int min =  1;
    int max = 10;
    
    int random_num = std::rand() % (max - min + 1);
    for (size_t i = 0; i < request_num; ++i) { 
        cache.LookUpUpdate(random_num, slow_get_page<int>);
    }
    
    EXPECT_LE(cache.hash_.size(), cache.hot_sz_ + cache.in_sz_ + cache.out_sz_);
}

int main(int argc, char* argv[]){

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

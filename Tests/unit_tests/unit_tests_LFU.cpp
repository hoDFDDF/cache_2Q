#include "LFU_cache.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

template <typename keyT> keyT slow_get_page(keyT key) { return key; }


TEST (lookUpUpdateTests, zeroCapacity) {
    cache::cache_LFU_<int, int> cache{0};

    EXPECT_FALSE(cache.lookUpUpdate(3, slow_get_page<int>));
}

TEST (lookUpUpdateTests, haveKeyAlready) {
    cache::cache_LFU_<int, int> cache{1};

    cache.lookUpUpdate(3, slow_get_page<int>);

    EXPECT_TRUE(cache.lookUpUpdate(3, slow_get_page<int>));
}

namespace cache {
    namespace {

        // Compare node addresses, not iterators from different containers.
        // The source cache must remain alive while checking a copy.
        template <typename Groups, typename Entries>
        testing::AssertionResult checkEntryIterators(const Groups& groups,
                                                   const Entries& entries) {
            std::size_t key_count = 0;
            for (const auto& group : groups) {
                for (const auto& key : group.keys) {
                    auto entry_it = entries.find(key);
                    if (entry_it == entries.end()) {
                        return testing::AssertionFailure() << "Missing entry for key " << key;
                    }

                    const auto& entry = entry_it->second;
                    if (std::addressof(*entry.group_it) != std::addressof(group)) {
                        return testing::AssertionFailure()
                            << "group_it for key " << key << " points outside its group";
                    }
                    if (std::addressof(*entry.key_it) != std::addressof(key)) {
                        return testing::AssertionFailure()
                            << "key_it for key " << key << " points outside its key node";
                    }
                    ++key_count;
                }
            }
            if (key_count != entries.size()) {
                return testing::AssertionFailure() << "Group and entry counts differ";
            }
            return testing::AssertionSuccess();
        }

        void fillCopySource(cache_LFU_<int, int>& lfu_cache) {
            for (int key = 1; key <= 3; ++key) {
                lfu_cache.lookUpUpdate(key, slow_get_page<int>);
            }
            lfu_cache.lookUpUpdate(1, slow_get_page<int>);
            lfu_cache.lookUpUpdate(2, slow_get_page<int>);
        }

    } // namespace

    TEST (lookUpUpdateTests, firstGroupDeleted) {
        cache::cache_LFU_<int, int> cache{5};

        cache.lookUpUpdate(1, slow_get_page<int>);
        cache.lookUpUpdate(2, slow_get_page<int>);
        cache.lookUpUpdate(3, slow_get_page<int>);
        cache.lookUpUpdate(4, slow_get_page<int>);
        cache.lookUpUpdate(5, slow_get_page<int>);

        cache.lookUpUpdate(1, slow_get_page<int>);
        cache.lookUpUpdate(2, slow_get_page<int>);
        cache.lookUpUpdate(3, slow_get_page<int>);
        cache.lookUpUpdate(4, slow_get_page<int>);
        cache.lookUpUpdate(5, slow_get_page<int>);

        EXPECT_EQ(cache.Fgroups_.size(), 1U);

        const auto& first_group = cache.Fgroups_.front();

        EXPECT_EQ(first_group.frequency, 2U);
        EXPECT_EQ(first_group.keys.size(), 5U);
    }

    TEST(LfuCacheTests, evictOneRemovesEmptyGroup) {
        cache_LFU_<int, int> lfu_cache{2};
        lfu_cache.lookUpUpdate(1, slow_get_page<int>);
        lfu_cache.lookUpUpdate(2, slow_get_page<int>);
        lfu_cache.lookUpUpdate(2, slow_get_page<int>);

        lfu_cache.evictOne();

        EXPECT_FALSE(lfu_cache.entries_.contains(1));
        EXPECT_TRUE(lfu_cache.entries_.contains(2));
        EXPECT_EQ(lfu_cache.entries_.size(), 1U);
        ASSERT_EQ(lfu_cache.Fgroups_.size(), 1U);
        EXPECT_EQ(lfu_cache.Fgroups_.front().frequency, 2U);
        EXPECT_THAT(lfu_cache.Fgroups_.front().keys, testing::ElementsAre(2));
        EXPECT_TRUE(checkEntryIterators(lfu_cache.Fgroups_, lfu_cache.entries_));
    }

    TEST(LfuCacheTests, evictOneRemovesLastEntry) {
        cache_LFU_<int, int> lfu_cache{1};
        lfu_cache.lookUpUpdate(1, slow_get_page<int>);

        lfu_cache.evictOne();

        EXPECT_TRUE(lfu_cache.entries_.empty());
        EXPECT_TRUE(lfu_cache.Fgroups_.empty());
        EXPECT_FALSE(lfu_cache.lookUpUpdate(2, slow_get_page<int>));
        EXPECT_TRUE(lfu_cache.lookUpUpdate(2, slow_get_page<int>));
    }

    TEST(LfuCacheTests, putStoresValuesAndIterators) {
        cache_LFU_<int, int> lfu_cache{3};

        lfu_cache.put(1, 10);
        lfu_cache.put(2, 20);

        ASSERT_EQ(lfu_cache.Fgroups_.size(), 1U);
        EXPECT_EQ(lfu_cache.Fgroups_.front().frequency, 1U);
        EXPECT_THAT(lfu_cache.Fgroups_.front().keys, testing::ElementsAre(2, 1));
        ASSERT_TRUE(checkEntryIterators(lfu_cache.Fgroups_, lfu_cache.entries_));
        EXPECT_EQ(lfu_cache.entries_.at(1).value, 10);
        EXPECT_EQ(lfu_cache.entries_.at(2).value, 20);

        // Remove frequency 1 by touching both keys, then create it again.
        lfu_cache.lookUpUpdate(1, slow_get_page<int>);
        lfu_cache.lookUpUpdate(2, slow_get_page<int>);
        lfu_cache.put(3, 30);

        ASSERT_EQ(lfu_cache.Fgroups_.size(), 2U);
        EXPECT_EQ(lfu_cache.Fgroups_.front().frequency, 1U);
        EXPECT_THAT(lfu_cache.Fgroups_.front().keys, testing::ElementsAre(3));
        EXPECT_EQ(lfu_cache.Fgroups_.back().frequency, 2U);
        EXPECT_THAT(lfu_cache.Fgroups_.back().keys, testing::ElementsAre(2, 1));
        ASSERT_TRUE(checkEntryIterators(lfu_cache.Fgroups_, lfu_cache.entries_));
        EXPECT_EQ(lfu_cache.entries_.at(3).value, 30);
    }

    TEST(LfuCacheTests, evictsOldestKeyInLowestFrequencyGroup) {
        cache_LFU_<int, int> lfu_cache{3};
        for (int key = 1; key <= 3; ++key) {
            lfu_cache.lookUpUpdate(key, slow_get_page<int>);
        }
        lfu_cache.lookUpUpdate(1, slow_get_page<int>);
        lfu_cache.lookUpUpdate(1, slow_get_page<int>);
        lfu_cache.lookUpUpdate(3, slow_get_page<int>);
        lfu_cache.lookUpUpdate(2, slow_get_page<int>);

        // frequency 2: [2, 3]; frequency 3: [1].
        // Key 1 is globally oldest, but its higher frequency protects it.
        // Key 3 was inserted after 2, but was last used before 2.
        ASSERT_EQ(lfu_cache.Fgroups_.size(), 2U);
        ASSERT_EQ(lfu_cache.Fgroups_.front().frequency, 2U);
        ASSERT_THAT(lfu_cache.Fgroups_.front().keys, testing::ElementsAre(2, 3));

        EXPECT_FALSE(lfu_cache.lookUpUpdate(4, slow_get_page<int>));

        EXPECT_FALSE(lfu_cache.entries_.contains(3));
        EXPECT_TRUE(lfu_cache.entries_.contains(1));
        EXPECT_TRUE(lfu_cache.entries_.contains(2));
        EXPECT_TRUE(lfu_cache.entries_.contains(4));
        EXPECT_EQ(lfu_cache.entries_.size(), 3U);
        ASSERT_EQ(lfu_cache.Fgroups_.size(), 3U);
        const auto& remaining_group = *std::next(lfu_cache.Fgroups_.begin());
        EXPECT_EQ(remaining_group.frequency, 2U);
        EXPECT_THAT(remaining_group.keys, testing::ElementsAre(2));
        EXPECT_TRUE(checkEntryIterators(lfu_cache.Fgroups_, lfu_cache.entries_));
    }

    TEST(LfuCacheTests, copyConstructorRebindsIterators) {
        cache_LFU_<int, int> source_cache{3};
        fillCopySource(source_cache);

        auto copied_cache = source_cache;

        EXPECT_EQ(copied_cache.capacity_, source_cache.capacity_);
        ASSERT_EQ(copied_cache.entries_.size(), 3U);
        ASSERT_EQ(copied_cache.Fgroups_.size(), 2U);
        ASSERT_TRUE(checkEntryIterators(copied_cache.Fgroups_, copied_cache.entries_));

        // Only mutate the copy after confirming that its iterators are its own.
        EXPECT_TRUE(copied_cache.lookUpUpdate(1, slow_get_page<int>));
        EXPECT_FALSE(copied_cache.lookUpUpdate(4, slow_get_page<int>));
        EXPECT_FALSE(copied_cache.entries_.contains(3));
        EXPECT_TRUE(source_cache.entries_.contains(3));
        EXPECT_FALSE(source_cache.entries_.contains(4));
        EXPECT_EQ(source_cache.entries_.at(1).group_it->frequency, 2U);
        EXPECT_EQ(copied_cache.entries_.at(1).group_it->frequency, 3U);
        EXPECT_TRUE(checkEntryIterators(source_cache.Fgroups_, source_cache.entries_));
        EXPECT_TRUE(checkEntryIterators(copied_cache.Fgroups_, copied_cache.entries_));
    }

    TEST(LfuCacheTests, copyAssignmentRebindsIterators) {
        cache_LFU_<int, int> source_cache{3};
        fillCopySource(source_cache);
        cache_LFU_<int, int> copied_cache{1};
        copied_cache.lookUpUpdate(99, slow_get_page<int>);

        copied_cache = source_cache;

        EXPECT_EQ(copied_cache.capacity_, source_cache.capacity_);
        EXPECT_FALSE(copied_cache.entries_.contains(99));
        ASSERT_EQ(copied_cache.entries_.size(), 3U);
        ASSERT_EQ(copied_cache.Fgroups_.size(), 2U);
        ASSERT_TRUE(checkEntryIterators(copied_cache.Fgroups_, copied_cache.entries_));

        EXPECT_TRUE(copied_cache.lookUpUpdate(1, slow_get_page<int>));
        EXPECT_EQ(source_cache.entries_.at(1).group_it->frequency, 2U);
        EXPECT_EQ(copied_cache.entries_.at(1).group_it->frequency, 3U);
        EXPECT_TRUE(checkEntryIterators(source_cache.Fgroups_, source_cache.entries_));
        EXPECT_TRUE(checkEntryIterators(copied_cache.Fgroups_, copied_cache.entries_));
    }
} // namespace cache

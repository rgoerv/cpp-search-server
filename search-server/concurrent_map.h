#pragma once
#include <algorithm>
#include <map>
#include <numeric>
#include <string>
#include <vector>
#include <mutex>
#include <iterator>

template <typename Key, typename Value>
class ConcurrentMap {
private:
    struct BucketMap {
        std::mutex map_guard;
        std::map<Key, Value> map_part;
    };
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");
    struct Access;

    Access operator[](const Key& key) {
        uint64_t position_in_vector = static_cast<uint64_t>(key) % buckets.size();
        return Access(buckets[position_in_vector].map_guard, buckets[position_in_vector].map_part, key);
    }

    struct Access {
        Access(std::mutex& value_mutex, std::map<Key, Value>& map, Key key) : access(value_mutex), ref_to_value(map[key]) {
        }
        std::lock_guard<std::mutex> access;
        Value& ref_to_value;
    };

    ConcurrentMap() = default;

    explicit ConcurrentMap(size_t bucket_count) : buckets(bucket_count) {
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> OrdinaryMap;
        size_t mutex_index = 0;

        for (const auto& solo_map : buckets) {
            std::lock_guard guard(buckets[mutex_index++].map_guard);
            OrdinaryMap.insert(solo_map.begin(), solo_map.end());
        }
        return OrdinaryMap;
    }

    size_t size() {
        return std::accumulate(buckets.begin(), buckets.end(), 0,
            [](const auto& container) { return container.size(); });
    }

    auto begin() {
        return buckets.begin();
    }

    auto end() {
        return buckets.end();
    }

    auto erase(const Key& key) {
        uint64_t position_in_vector = static_cast<uint64_t>(key) % buckets.size();
        std::lock_guard guard(buckets[position_in_vector].map_guard);
        return buckets[position_in_vector].map_part.erase(key);
    }

private:
    std::vector<BucketMap> buckets;
};
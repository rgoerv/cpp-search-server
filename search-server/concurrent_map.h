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
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");
    struct Access;

    Access operator[](const Key& key) {
        uint64_t position_in_vector = static_cast<uint64_t>(key) % bucket_maps_.size();
        return Access(map_mutexs_[position_in_vector], bucket_maps_[position_in_vector], key);
    }

    struct Access {
        Access(std::mutex& value_mutex, std::map<Key, Value>& map, Key key) : access(value_mutex), ref_to_value(map[key]) {
        }
        std::lock_guard<std::mutex> access;
        Value& ref_to_value;
    };

    ConcurrentMap() = default;

    explicit ConcurrentMap(size_t bucket_count) : map_mutexs_(bucket_count), bucket_maps_(bucket_count) {
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> OrdinaryMap;
        size_t mutex_index = 0;

        for (const auto& solo_map : bucket_maps_) {
            lock_guard guard(map_mutexs_[mutex_index++]);
            OrdinaryMap.insert(solo_map.begin(), solo_map.end());
        }
        return OrdinaryMap;
    }

    size_t size() {
        size_t size = 0;
        for (const auto& container : bucket_maps_) {
            size += container.size();
        }
        return size;
    }

    auto begin() {
        return bucket_maps_.begin();
    }

    auto end() {
        return bucket_maps_.end();
    }
    
    auto erase(const Key& key) {
        uint64_t position_in_vector = static_cast<uint64_t>(key) % bucket_maps_.size();
        std::lock_guard guard(map_mutexs_[position_in_vector]);
        return bucket_maps_[position_in_vector].erase(key);
    }

    //Access operator+=(Access& lhs, Value& value) {
    //    return lhs.ref_to_value += value;
    //}

private:
    std::vector<std::mutex> map_mutexs_;
    std::vector<std::map<Key, Value>> bucket_maps_;
};
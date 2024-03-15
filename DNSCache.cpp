// DNSCache.cpp

#include "DNSCache.h"
#include <iostream>

// O(1) on insertion and update on avg
void DNSCache::update(const std::string& name, const std::string& ip) {
    std::unique_lock<std::shared_mutex> writer_lock(m_sh_mutex); // Only one thread/writer can write to cache

    auto it = m_hash_cache.find(name);
    if (it != m_hash_cache.end()) { // Found the entry
        it->second->ip = ip; // Update only relevant parts
        if (it->second != m_lru_cache.begin()) { // Send to LRU beginning
            m_lru_cache.splice(m_lru_cache.begin(), m_lru_cache, it->second);
        }
    }
    else { // New entry
        if (m_hash_cache.size() == m_max_size) {
            m_hash_cache.erase(m_lru_cache.back().domain);
            m_lru_cache.pop_back();
        }
        m_lru_cache.push_front({ name, ip });
        m_hash_cache[name] = m_lru_cache.begin();
    }
}

// O(1) for search on avg
std::string DNSCache::resolve(const std::string& name) {
    std::shared_lock<std::shared_mutex> reader_lock(m_sh_mutex); // Multiple threads/readers can read cache at the same time

    auto it = m_hash_cache.find(name);
    if (it == m_hash_cache.end()) {
        return ""; // Not found
    }

    {
        std::lock_guard lru_lock(m_mutex); // Protect LRU cache from concurrent reading threads 
        if (it->second != m_lru_cache.begin()) { // Send to LRU beginning
            m_lru_cache.splice(m_lru_cache.begin(), m_lru_cache, it->second);
        }
    }

    return it->second->ip; // Safe to return after unlock in concurrent reading threads
}

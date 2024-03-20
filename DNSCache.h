// DNSCache.h

#pragma once

#include <exception>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <memory>

class DNSCache {
public:
    explicit DNSCache(size_t max_size) : m_max_size(max_size) {
        if (!(max_size > 0))
            throw std::runtime_error("Expected DNSCache max_size > 0");
    }

    DNSCache(DNSCache const&) = delete;
    DNSCache(DNSCache&&) = delete;
    DNSCache& operator=(DNSCache const&) = delete;
    DNSCache& operator=(DNSCache&&) = delete;

    void update(const std::string& name, const std::string& ip);
    std::string resolve(const std::string& name);

private:
    size_t m_max_size;
    mutable std::mutex m_mutex;
    mutable std::shared_mutex m_sh_mutex;

    struct DomainInfo {
        std::string domain;
        std::string ip;
    };

    std::list<DomainInfo> m_lru_cache; // To track least recently used and stable iterators

    using DomainInfoListIter = std::list<DomainInfo>::iterator;
    std::unordered_map<std::string_view, DomainInfoListIter> m_hash_cache;
};

class DNSCacheSingleton {
public:
    static DNSCache& getInstance(size_t max_size) {
        static DNSCache instance(max_size); // thread-safe init
        return instance;
    }

    DNSCacheSingleton(DNSCache const&) = delete;
    DNSCacheSingleton(DNSCache&&) = delete;
    DNSCacheSingleton& operator=(DNSCache const&) = delete;
    DNSCacheSingleton& operator=(DNSCache&&) = delete;

private:
    explicit DNSCacheSingleton() {};
};
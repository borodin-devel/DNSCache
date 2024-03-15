#include <chrono>
#include <thread>
#include <memory>
#include <vector>
#include <gtest/gtest.h>
#include "DNSCache.h"

// Change crazyFactor in tests to test stress load

TEST(DNSCacheTest, ResolveNonExistent) {
    auto dnsCache = DNSCache(1);
    EXPECT_EQ(dnsCache.resolve("nonexistent.com"), "");
}

TEST(DNSCacheTest, UpdateAndResolve) {
    auto dnsCache = DNSCache(1);
    dnsCache.update("example.com", "1.2.3.4");
    EXPECT_EQ(dnsCache.resolve("example.com"), "1.2.3.4");
}

TEST(DNSCacheTest, UpdateExistingEntry) {
    auto dnsCache = DNSCache(1);
    dnsCache.update("example.com", "1.2.3.4");
    dnsCache.update("example.com", "5.6.7.8");
    EXPECT_EQ(dnsCache.resolve("example.com"), "5.6.7.8");
}

TEST(DNSCacheTest, LRUCacheEviction) {
    auto dnsCache = DNSCache(3);
    dnsCache.update("example1.com", "1.2.3.4");
    dnsCache.update("example2.com", "5.6.7.8");
    dnsCache.update("example3.com", "5.6.7.8");
    dnsCache.resolve("example1.com");
    dnsCache.resolve("example3.com");
    dnsCache.update("example4.com", "9.10.11.12");

    EXPECT_EQ(dnsCache.resolve("example1.com"), "1.2.3.4");
    EXPECT_EQ(dnsCache.resolve("example2.com"), ""); // example2.com should be evicted
    EXPECT_EQ(dnsCache.resolve("example3.com"), "5.6.7.8");
    EXPECT_EQ(dnsCache.resolve("example4.com"), "9.10.11.12");
}

class DNSCacheConcurrencyTest : public ::testing::Test {
protected:
    std::unique_ptr<DNSCache> dnsCache;
    static constexpr size_t numThreads = 8;
    static constexpr size_t crazyFactor = 1000;
    static constexpr size_t multiplier = 80 * crazyFactor;
    static constexpr size_t cacheSize = numThreads * multiplier;

    void SetUp() override {
        // Ensure each test starts with a fresh DNSCache instance
        dnsCache.reset();
        dnsCache = std::make_unique<DNSCache>(cacheSize);
    }

    void TearDown() override {
        // Clean up after each test
        dnsCache.reset();
    }

    // Utility function to join all threads
    void joinThreads(std::vector<std::thread>& threads) {
        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }
    }
};

TEST_F(DNSCacheConcurrencyTest, ConcurrentUpdates) {
    std::vector<std::thread> threads;
    auto startTime = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < numThreads; ++i) {
        threads.push_back(std::thread([this, i]() {
            for (size_t j = i * multiplier; j < (i + 1) * multiplier; ++j) {
                std::string domain = "example" + std::to_string(j) + ".com";
                dnsCache->update(domain, "1.2.3." + std::to_string(j));
            }
            }));
    }
    joinThreads(threads);
    auto endTime = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    std::cout << "Cache size: " << cacheSize << '\n';
    std::cout << "Updates time: " << duration << "ms" << std::endl;

    // Verify all entries
    for (size_t i = 0; i < cacheSize; ++i) {
        std::string domain = "example" + std::to_string(i) + ".com";
        EXPECT_NE(dnsCache->resolve(domain), "");
    }
}

TEST_F(DNSCacheConcurrencyTest, ConcurrentResolves) {
    // Pre-populate the cache
    for (size_t i = 0; i < cacheSize; ++i) {
        std::string domain = "example" + std::to_string(i) + ".com";
        dnsCache->update(domain, "1.2.3." + std::to_string(i));
    }

    std::vector<std::thread> threads;
    auto startTime = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < numThreads; ++i) {
        threads.push_back(std::thread([this, i]() {
            for (size_t j = i * multiplier; j < (i + 1) * multiplier; ++j) {
                std::string domain = "example" + std::to_string(j) + ".com";
                EXPECT_EQ(dnsCache->resolve(domain), "1.2.3." + std::to_string(j));
            }
            }));
    }
    joinThreads(threads);
    auto endTime = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    std::cout << "Cache size: " << cacheSize << '\n';
    std::cout << "Resolves (with expect) time: " << duration << "ms" << std::endl;
}

TEST_F(DNSCacheConcurrencyTest, ConcurrentUpdatesAndResolves) {
    for (size_t i = 0; i < cacheSize; ++i) {
        std::string domain = "example" + std::to_string(i) + ".com";
        dnsCache->update(domain, "1.2.3." + std::to_string(i));
    }

    std::vector<std::thread> updaterThreads, resolverThreads;
    auto startTime = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < numThreads; ++i) {
        updaterThreads.push_back(std::thread([this, i]() {
            for (size_t j = i * multiplier; j < (i + 1) * multiplier; ++j) {
                std::string domain = "example" + std::to_string(j) + ".com";
                dnsCache->update(domain, "4.5.6." + std::to_string(j));
            }
            }));
    }

    for (size_t i = 0; i < numThreads; ++i) {
        resolverThreads.push_back(std::thread([this, i]() {
            for (size_t j = i * multiplier; j < (i + 1) * multiplier; ++j) {
                std::string domain = "example" + std::to_string(j) + ".com";
                std::string ip = dnsCache->resolve(domain);
                // Assert that we get either the old or new IP
                bool isValidIP = ip == "1.2.3." + std::to_string(j) || ip == "4.5.6." + std::to_string(j);
                EXPECT_TRUE(isValidIP);
            }
            }));
    }
    joinThreads(updaterThreads);
    joinThreads(resolverThreads);
    auto endTime = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    std::cout << "Cache size: " << cacheSize << '\n';
    std::cout << "Updates and resolves (with expect) time: " << duration << "ms" << std::endl;
}

TEST_F(DNSCacheConcurrencyTest, ResolvePerformanceUnderLoad) {
    for (size_t i = 0; i < cacheSize; ++i) {
        std::string domain = "example" + std::to_string(i) + ".com";
        dnsCache->update(domain, "1.2.3." + std::to_string(i));
    }

    auto startTime = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> threads;
    for (size_t i = 0; i < numThreads; ++i) {
        threads.push_back(std::thread([this, i]() {
            for (size_t j = i * multiplier; j < (i + 1) * multiplier; ++j) {
                dnsCache->resolve("example" + std::to_string(j) + ".com");
            }
            }));
    }
    joinThreads(threads);
    auto endTime = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    std::cout << "Cache size: " << cacheSize << '\n';
    std::cout << "Resolution time under load (without expect): " << duration << "ms" << std::endl;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
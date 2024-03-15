#include "DNSCache.h"

#include <iostream>

int main()
{
	auto& dnsCache = DNSCacheSingleton::getInstance(3);

	dnsCache.update("example.com", "1.2.3.4");
	dnsCache.update("example2.com", "5.6.7.8");
	dnsCache.update("example3.com", "9.10.11.12");

	std::cout << dnsCache.resolve("example.com");

	return 0;
}
#ifndef SINGLE_THREAD_PORXY_CACHE_H
#define SINGLE_THREAD_PORXY_CACHE_H

#include <map>
#include <string>
#include "CacheEntry.h"


class Cache {
public:


private:
    std::map<std::string, CacheEntry> urlToCacheEntry;
};


#endif //SINGLE_THREAD_PORXY_CACHE_H

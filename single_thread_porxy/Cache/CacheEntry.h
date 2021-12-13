//
// Created by yevsyukof on 08.12.2021.
//

#ifndef SINGLE_THREAD_PORXY_CACHEENTRY_H
#define SINGLE_THREAD_PORXY_CACHEENTRY_H

#include <utility>
#include <vector>
#include <memory>

class CacheEntry {
public:
    CacheEntry() = default;

    explicit CacheEntry(const std::shared_ptr<std::vector<char>> &entryData) {
        data = entryData;
    }

    const std::shared_ptr<std::vector<char>> &getCacheEntryData() const {
        return data;
    }

private:
    std::shared_ptr<std::vector<char>> data;
};


#endif //SINGLE_THREAD_PORXY_CACHEENTRY_H

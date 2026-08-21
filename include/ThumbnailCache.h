#pragma once

#include <Windows.h>
#include <string>
#include <unordered_map>
#include <list>
#include <mutex>
#include <chrono>

struct CachedThumbnail {
    HBITMAP hBitmap = nullptr;
    int width = 0;
    int height = 0;
    std::wstring filePath;
    std::chrono::steady_clock::time_point lastAccess;
    uint64_t fileSize = 0;
    FILETIME lastWriteTime{};
};

class ThumbnailCache {
public:
    ThumbnailCache(size_t maxEntries = 100, size_t maxMemoryMB = 50);
    ~ThumbnailCache();

    // Get thumbnail from cache or return nullptr
    CachedThumbnail* Get(const std::wstring& filePath);

    // Add thumbnail to cache (takes ownership of HBITMAP)
    void Put(const std::wstring& filePath, HBITMAP hBitmap, int width, int height, uint64_t fileSize, FILETIME ft);

    // Check if cache has entry (even if stale)
    bool Has(const std::wstring& filePath);

    // Remove entry
    void Remove(const std::wstring& filePath);

    // Clear all entries
    void Clear();

    // Get cache stats
    size_t GetEntryCount() const;
    size_t GetMemoryUsage() const;

private:
    void EvictIfNeeded();
    void FreeEntry(CachedThumbnail& entry);

    size_t m_maxEntries;
    size_t m_maxMemoryBytes;
    size_t m_currentMemoryBytes = 0;

    std::unordered_map<std::wstring, std::list<CachedThumbnail>::iterator> m_map;
    std::list<CachedThumbnail> m_list; // LRU: front = most recent
    mutable std::mutex m_mutex;
};

#include "ThumbnailCache.h"
#include <algorithm>

ThumbnailCache::ThumbnailCache(size_t maxEntries, size_t maxMemoryMB) 
    : m_maxEntries(maxEntries), m_maxMemoryBytes(maxMemoryMB * 1024 * 1024) {
}

ThumbnailCache::~ThumbnailCache() {
    Clear();
}

CachedThumbnail* ThumbnailCache::Get(const std::wstring& filePath) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_map.find(filePath);
    if (it == m_map.end()) return nullptr;

    // Move to front (most recently used)
    m_list.splice(m_list.begin(), m_list, it->second);
    it->second->lastAccess = std::chrono::steady_clock::now();

    return &(*it->second);
}

bool ThumbnailCache::Has(const std::wstring& filePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_map.find(filePath) != m_map.end();
}

void ThumbnailCache::Put(const std::wstring& filePath, HBITMAP hBitmap, int width, int height, uint64_t fileSize, FILETIME ft) {
    if (!hBitmap) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    // Remove existing entry if present
    auto existing = m_map.find(filePath);
    if (existing != m_map.end()) {
        FreeEntry(*existing->second);
        m_list.erase(existing->second);
        m_map.erase(existing);
    }

    // Estimate memory: width * height * 4 bytes (32bpp)
    size_t entryMemory = static_cast<size_t>(width) * height * 4;

    // Evict if needed
    while ((m_map.size() >= m_maxEntries || m_currentMemoryBytes + entryMemory > m_maxMemoryBytes) && !m_list.empty()) {
        FreeEntry(m_list.back());
        m_map.erase(m_list.back().filePath);
        m_currentMemoryBytes -= static_cast<size_t>(m_list.back().width) * m_list.back().height * 4;
        m_list.pop_back();
    }

    // Add new entry
    CachedThumbnail entry;
    entry.hBitmap = hBitmap;
    entry.width = width;
    entry.height = height;
    entry.filePath = filePath;
    entry.fileSize = fileSize;
    entry.lastWriteTime = ft;
    entry.lastAccess = std::chrono::steady_clock::now();

    m_list.push_front(entry);
    m_map[filePath] = m_list.begin();
    m_currentMemoryBytes += entryMemory;
}

void ThumbnailCache::Remove(const std::wstring& filePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_map.find(filePath);
    if (it != m_map.end()) {
        FreeEntry(*it->second);
        m_currentMemoryBytes -= static_cast<size_t>(it->second->width) * it->second->height * 4;
        m_list.erase(it->second);
        m_map.erase(it);
    }
}

void ThumbnailCache::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& entry : m_list) {
        FreeEntry(entry);
    }
    m_list.clear();
    m_map.clear();
    m_currentMemoryBytes = 0;
}

void ThumbnailCache::FreeEntry(CachedThumbnail& entry) {
    if (entry.hBitmap) {
        DeleteObject(entry.hBitmap);
        entry.hBitmap = nullptr;
    }
}

size_t ThumbnailCache::GetEntryCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_map.size();
}

size_t ThumbnailCache::GetMemoryUsage() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentMemoryBytes;
}

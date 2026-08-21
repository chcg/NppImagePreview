#pragma once

#include <string>

class PathResolver {
public:
    PathResolver() = default;

    // Resolve relative path against current document path
    std::wstring ResolvePath(const std::wstring& rawPath, const std::wstring& currentFilePath);

    // Check if file exists
    static bool FileExists(const std::wstring& path);

    // Check if path is a remote URL
    static bool IsRemoteUrl(const std::wstring& path);

    // Get file size as human-readable string
    static std::wstring GetFileSizeString(const std::wstring& path);

    // Get file dimensions if image
    static std::pair<int, int> GetImageDimensions(const std::wstring& path);
};

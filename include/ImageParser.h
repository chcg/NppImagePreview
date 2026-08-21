#include <Windows.h>
#pragma once

#include <string>
#include <vector>
#include <regex>

// Supported image extensions
const wchar_t* const IMAGE_EXTENSIONS[] = {
    L"png", L"jpg", L"jpeg", L"webp", L"gif", 
    L"svg", L"bmp", L"ico", L"avif", L"tiff", L"tif"
};

struct ImageRef {
    int line = -1;
    int colStart = -1;
    int colEnd = -1;
    std::wstring rawPath;
    std::wstring resolvedPath;
    bool exists = false;
    bool isRemote = false;
};

class ImageParser {
public:
    ImageParser();

    // Parse visible lines in Scintilla editor
    std::vector<ImageRef> ParseVisibleLines(HWND hSci, const std::wstring& currentFilePath);

private:
    bool HasImageExtension(const std::wstring& path);
    std::wstring ExtractPath(const std::wstring& lineText);

    // Regex patterns for different contexts
    std::wregex m_htmlSrcRegex;
    std::wregex m_cssUrlRegex;
    std::wregex m_phpStringRegex;
    std::wregex m_jsSrcRegex;
    std::wregex m_mdImageRegex;
    std::wregex m_genericPathRegex;
};

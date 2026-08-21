#define NOMINMAX
#include "Scintilla.h"
#include "ImageParser.h"
#include <algorithm>
#include <cctype>
#include <sstream>

ImageParser::ImageParser() {
    m_htmlSrcRegex = std::wregex(LR"(src\s*=\s*["']([^"']+\.(?:png|jpg|jpeg|webp|gif|svg|bmp|ico|avif|tiff|tif))["'])", std::regex::icase);
    m_cssUrlRegex = std::wregex(LR"(url\s*\(\s*["']?([^"')]+\.(?:png|jpg|jpeg|webp|gif|svg|bmp|ico|avif|tiff|tif))["']?\s*\))", std::regex::icase);
    m_phpStringRegex = std::wregex(LR"((?:echo|print|\$\w+)\s*.*["']([^"']+\.(?:png|jpg|jpeg|webp|gif|svg|bmp|ico|avif|tiff|tif))["'])", std::regex::icase);
    m_jsSrcRegex = std::wregex(LR"(\.src\s*=\s*["']([^"']+\.(?:png|jpg|jpeg|webp|gif|svg|bmp|ico|avif|tiff|tif))["'])", std::regex::icase);
    m_mdImageRegex = std::wregex(LR"(!\[[^\]]*\]\(([^)]+\.(?:png|jpg|jpeg|webp|gif|svg|bmp|ico|avif|tiff|tif))\))", std::regex::icase);
    m_genericPathRegex = std::wregex(LR"(["']([^"']+\.(?:png|jpg|jpeg|webp|gif|svg|bmp|ico|avif|tiff|tif))["'])", std::regex::icase);
}

static void ParserLog(const wchar_t* msg) {
    OutputDebugStringW(L"[Parser] ");
    OutputDebugStringW(msg);
    OutputDebugStringW(L"\n");
}

bool ImageParser::HasImageExtension(const std::wstring& path) {
    size_t dot = path.find_last_of(L'.');
    if (dot == std::wstring::npos) return false;

    std::wstring ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    for (const auto& validExt : IMAGE_EXTENSIONS) {
        if (ext == validExt) return true;
    }
    return false;
}

std::wstring ImageParser::ExtractPath(const std::wstring& lineText) {
    std::wsmatch match;

    try {
        if (std::regex_search(lineText, match, m_htmlSrcRegex) && match.size() > 1) {
            ParserLog(L"Regex matched: HTML src");
            return match[1].str();
        }
        if (std::regex_search(lineText, match, m_cssUrlRegex) && match.size() > 1) {
            ParserLog(L"Regex matched: CSS url");
            return match[1].str();
        }
        if (std::regex_search(lineText, match, m_mdImageRegex) && match.size() > 1) {
            ParserLog(L"Regex matched: Markdown");
            return match[1].str();
        }
        if (std::regex_search(lineText, match, m_jsSrcRegex) && match.size() > 1) {
            ParserLog(L"Regex matched: JS src");
            return match[1].str();
        }
        if (std::regex_search(lineText, match, m_phpStringRegex) && match.size() > 1) {
            ParserLog(L"Regex matched: PHP");
            return match[1].str();
        }
        if (std::regex_search(lineText, match, m_genericPathRegex) && match.size() > 1) {
            ParserLog(L"Regex matched: Generic path");
            return match[1].str();
        }
    } catch (const std::regex_error&) {
        ParserLog(L"Regex exception caught!");
    }

    return L"";
}

std::vector<ImageRef> ImageParser::ParseVisibleLines(HWND hSci, const std::wstring& currentFilePath) {
    std::vector<ImageRef> results;

    if (!hSci) {
        ParserLog(L"ParseVisibleLines: hSci is null");
        return results;
    }

    LRESULT firstVisible = ::SendMessage(hSci, SCI_GETFIRSTVISIBLELINE, 0, 0);
    LRESULT linesOnScreen = ::SendMessage(hSci, SCI_LINESONSCREEN, 0, 0);
    LRESULT totalLines = ::SendMessage(hSci, SCI_GETLINECOUNT, 0, 0);

    LRESULT startLine = firstVisible;
    LRESULT endLine = std::min(firstVisible + linesOnScreen + 2, totalLines);

    wchar_t rangeBuf[128];
    swprintf_s(rangeBuf, L"ParseVisibleLines: lines %I64d to %I64d (total %I64d)", startLine, endLine, totalLines);
    ParserLog(rangeBuf);

    for (LRESULT line = startLine; line < endLine; ++line) {
        LRESULT lineLen = ::SendMessage(hSci, SCI_LINELENGTH, line, 0);
        if (lineLen <= 0 || lineLen > 4096) continue;

        char buffer[4096] = {0};
        LRESULT copied = ::SendMessage(hSci, SCI_GETLINE, line, reinterpret_cast<LPARAM>(buffer));
        if (copied <= 0) continue;

        std::string lineStr(buffer, copied);

        // Try UTF-8 first, then ANSI fallback
        int wlen = ::MultiByteToWideChar(CP_UTF8, 0, lineStr.c_str(), static_cast<int>(lineStr.length()), nullptr, 0);
        if (wlen <= 0) {
            wlen = ::MultiByteToWideChar(CP_ACP, 0, lineStr.c_str(), static_cast<int>(lineStr.length()), nullptr, 0);
        }
        if (wlen <= 0) {
            ParserLog(L"MultiByteToWideChar failed for a line");
            continue;
        }

        std::wstring wline(wlen, 0);
        int conv = ::MultiByteToWideChar(CP_UTF8, 0, lineStr.c_str(), static_cast<int>(lineStr.length()), &wline[0], wlen);
        if (conv <= 0) {
            conv = ::MultiByteToWideChar(CP_ACP, 0, lineStr.c_str(), static_cast<int>(lineStr.length()), &wline[0], wlen);
        }
        if (conv <= 0) continue;

        // Remove trailing \r and \n for cleaner regex
        while (!wline.empty() && (wline.back() == L'\r' || wline.back() == L'\n')) {
            wline.pop_back();
        }

        // Only log lines that contain a dot (potential file path)
        if (wline.find(L'.') != std::wstring::npos) {
            wchar_t logBuf[512];
            swprintf_s(logBuf, L"Line %I64d text: [%s]", line, wline.c_str());
            ParserLog(logBuf);
        }

        std::wstring path = ExtractPath(wline);
        if (path.empty()) continue;

        wchar_t logBuf[512];
        swprintf_s(logBuf, L"Line %I64d EXTRACTED: [%s]", line, path.c_str());
        ParserLog(logBuf);

        size_t pos = wline.find(path);
        if (pos == std::wstring::npos) continue;

        ImageRef ref;
        ref.line = static_cast<int>(line);
        ref.colStart = static_cast<int>(pos);
        ref.colEnd = static_cast<int>(pos + path.length());
        ref.rawPath = path;

        results.push_back(ref);
    }

    swprintf_s(rangeBuf, L"ParseVisibleLines: returning %zu result(s)", results.size());
    ParserLog(rangeBuf);
    return results;
}
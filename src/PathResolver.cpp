#include "PathResolver.h"
#include <Windows.h>
#include <shlwapi.h>
#include <wincodec.h>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "windowscodecs.lib")

std::wstring PathResolver::ResolvePath(const std::wstring& rawPath, const std::wstring& currentFilePath) {
    if (rawPath.empty()) return L"";

    // Check if remote URL
    if (IsRemoteUrl(rawPath)) {
        return rawPath;
    }

    // If already absolute
    if (PathIsRelativeW(rawPath.c_str()) == FALSE) {
        return rawPath;
    }

    // Get directory of current file
    wchar_t dir[MAX_PATH];
    wcsncpy_s(dir, currentFilePath.c_str(), MAX_PATH);
    PathRemoveFileSpecW(dir);

    // Resolve relative path
    wchar_t resolved[MAX_PATH];
    if (PathCombineW(resolved, dir, rawPath.c_str())) {
        wchar_t fullPath[MAX_PATH];
        if (GetFullPathNameW(resolved, MAX_PATH, fullPath, nullptr)) {
            return fullPath;
        }
    }

    return L"";
}

bool PathResolver::FileExists(const std::wstring& path) {
    if (path.empty() || IsRemoteUrl(path)) return false;
    DWORD attribs = GetFileAttributesW(path.c_str());
    return (attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY));
}

bool PathResolver::IsRemoteUrl(const std::wstring& path) {
    return path.find(L"http://") == 0 || path.find(L"https://") == 0 || 
           path.find(L"ftp://") == 0 || path.find(L"//") == 0;
}

std::wstring PathResolver::GetFileSizeString(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fad)) {
        return L"Unknown";
    }

    ULONGLONG size = (static_cast<ULONGLONG>(fad.nFileSizeHigh) << 32) | fad.nFileSizeLow;

    std::wostringstream oss;
    if (size < 1024) {
        oss << size << L" B";
    } else if (size < 1024 * 1024) {
        oss << std::fixed << std::setprecision(1) << (size / 1024.0) << L" KB";
    } else if (size < 1024 * 1024 * 1024) {
        oss << std::fixed << std::setprecision(1) << (size / (1024.0 * 1024.0)) << L" MB";
    } else {
        oss << std::fixed << std::setprecision(2) << (size / (1024.0 * 1024.0 * 1024.0)) << L" GB";
    }
    return oss.str();
}

std::pair<int, int> PathResolver::GetImageDimensions(const std::wstring& path) {
    IWICImagingFactory* pFactory = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IWICImagingFactory,
        reinterpret_cast<LPVOID*>(&pFactory)
    );

    if (FAILED(hr)) return {0, 0};

    IWICBitmapDecoder* pDecoder = nullptr;
    hr = pFactory->CreateDecoderFromFilename(
        path.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnDemand,
        &pDecoder
    );

    if (FAILED(hr)) {
        pFactory->Release();
        return {0, 0};
    }

    IWICBitmapFrameDecode* pFrame = nullptr;
    hr = pDecoder->GetFrame(0, &pFrame);

    UINT width = 0, height = 0;
    if (SUCCEEDED(hr) && pFrame) {
        pFrame->GetSize(&width, &height);
        pFrame->Release();
    }

    pDecoder->Release();
    pFactory->Release();

    return {static_cast<int>(width), static_cast<int>(height)};
}

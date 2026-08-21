#pragma once

#include <Windows.h>
#include <wincodec.h>
#include <string>

#pragma comment(lib, "windowscodecs.lib")

struct LoadedImage {
    HBITMAP hBitmap = nullptr;
    int width = 0;
    int height = 0;
    std::wstring format;
    bool success = false;
};

class ImageLoader {
public:
    ImageLoader();
    ~ImageLoader();

    bool Initialize();
    void Shutdown();

    // Load and resize image to max dimension
    LoadedImage LoadThumbnail(const std::wstring& filePath, int maxDimension = 200);

    // Get image info without loading pixels
    bool GetImageInfo(const std::wstring& filePath, int& width, int& height, std::wstring& format);

private:
    IWICImagingFactory* m_pFactory = nullptr;
    bool m_initialized = false;

    HBITMAP CreateHBitmapFromWicBitmap(IWICBitmapSource* pSource, int width, int height);
    WICPixelFormatGUID GetTargetPixelFormat(const WICPixelFormatGUID& sourceFormat);
};

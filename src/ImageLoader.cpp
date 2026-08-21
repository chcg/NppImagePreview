#include "ImageLoader.h"
#include <wincodec.h>
#include <gdiplus.h>
#include <memory>

#pragma comment(lib, "gdiplus.lib")

ImageLoader::ImageLoader() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
}

ImageLoader::~ImageLoader() {
    Shutdown();
    CoUninitialize();
}

bool ImageLoader::Initialize() {
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IWICImagingFactory,
        reinterpret_cast<LPVOID*>(&m_pFactory)
    );

    if (SUCCEEDED(hr)) {
        m_initialized = true;
        return true;
    }
    return false;
}

void ImageLoader::Shutdown() {
    if (m_pFactory) {
        m_pFactory->Release();
        m_pFactory = nullptr;
    }
    m_initialized = false;
}

WICPixelFormatGUID ImageLoader::GetTargetPixelFormat(const WICPixelFormatGUID& sourceFormat) {
    // Prefer 32bpp BGRA for GDI compatibility
    if (sourceFormat == GUID_WICPixelFormat32bppBGRA ||
        sourceFormat == GUID_WICPixelFormat32bppPBGRA) {
        return GUID_WICPixelFormat32bppBGRA;
    }
    return GUID_WICPixelFormat32bppBGR;
}

HBITMAP ImageLoader::CreateHBitmapFromWicBitmap(IWICBitmapSource* pSource, int width, int height) {
    if (!pSource) return nullptr;

    // Create compatible DC
    HDC hScreenDC = GetDC(nullptr);
    HDC hMemDC = CreateCompatibleDC(hScreenDC);

    // Create DIB section
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(hMemDC, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);

    if (!hBitmap || !pBits) {
        DeleteDC(hMemDC);
        ReleaseDC(nullptr, hScreenDC);
        return nullptr;
    }

    // Copy WIC pixels to DIB
    UINT stride = width * 4;
    UINT bufferSize = stride * height;

    HRESULT hr = pSource->CopyPixels(nullptr, stride, bufferSize, static_cast<BYTE*>(pBits));

    DeleteDC(hMemDC);
    ReleaseDC(nullptr, hScreenDC);

    if (FAILED(hr)) {
        DeleteObject(hBitmap);
        return nullptr;
    }

    return hBitmap;
}

LoadedImage ImageLoader::LoadThumbnail(const std::wstring& filePath, int maxDimension) {
    LoadedImage result;

    if (!m_initialized || filePath.empty()) return result;

    // Skip remote URLs for now
    if (filePath.find(L"http://") == 0 || filePath.find(L"https://") == 0) {
        result.success = false;
        return result;
    }

    IWICBitmapDecoder* pDecoder = nullptr;
    HRESULT hr = m_pFactory->CreateDecoderFromFilename(
        filePath.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnDemand,
        &pDecoder
    );

    if (FAILED(hr) || !pDecoder) return result;

    // Get container format for file type
    GUID containerFormat;
    hr = pDecoder->GetContainerFormat(&containerFormat);
    if (SUCCEEDED(hr)) {
        if (containerFormat == GUID_ContainerFormatPng) result.format = L"PNG";
        else if (containerFormat == GUID_ContainerFormatJpeg) result.format = L"JPEG";
        else if (containerFormat == GUID_ContainerFormatGif) result.format = L"GIF";
        else if (containerFormat == GUID_ContainerFormatBmp) result.format = L"BMP";
        else if (containerFormat == GUID_ContainerFormatTiff) result.format = L"TIFF";
        else if (containerFormat == GUID_ContainerFormatWmp) result.format = L"WMP";
        else if (containerFormat == GUID_ContainerFormatIco) result.format = L"ICO";
        else result.format = L"Image";
    }

    IWICBitmapFrameDecode* pFrame = nullptr;
    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr) || !pFrame) {
        pDecoder->Release();
        return result;
    }

    UINT origWidth = 0, origHeight = 0;
    hr = pFrame->GetSize(&origWidth, &origHeight);
    if (FAILED(hr) || origWidth == 0 || origHeight == 0) {
        pFrame->Release();
        pDecoder->Release();
        return result;
    }

    result.width = static_cast<int>(origWidth);
    result.height = static_cast<int>(origHeight);

    // Calculate thumbnail dimensions maintaining aspect ratio
    int thumbWidth = static_cast<int>(origWidth);
    int thumbHeight = static_cast<int>(origHeight);

    if (thumbWidth > maxDimension || thumbHeight > maxDimension) {
        double ratio = static_cast<double>(origWidth) / origHeight;
        if (ratio > 1.0) {
            thumbWidth = maxDimension;
            thumbHeight = static_cast<int>(maxDimension / ratio);
        } else {
            thumbHeight = maxDimension;
            thumbWidth = static_cast<int>(maxDimension * ratio);
        }
    }

    // Convert pixel format
    WICPixelFormatGUID pixelFormat;
    hr = pFrame->GetPixelFormat(&pixelFormat);

    IWICBitmapSource* pConverted = nullptr;
    if (SUCCEEDED(hr)) {
        IWICFormatConverter* pConverter = nullptr;
        hr = m_pFactory->CreateFormatConverter(&pConverter);
        if (SUCCEEDED(hr) && pConverter) {
            WICPixelFormatGUID targetFormat = GetTargetPixelFormat(pixelFormat);
            hr = pConverter->Initialize(
                pFrame,
                targetFormat,
                WICBitmapDitherTypeNone,
                nullptr,
                0.0,
                WICBitmapPaletteTypeCustom
            );
            if (SUCCEEDED(hr)) {
                pConverted = pConverter;
            } else {
                pConverter->Release();
            }
        }
    }

    // Scale if needed
    IWICBitmapSource* pFinal = pConverted;
    IWICBitmapScaler* pScaler = nullptr;

    if (pConverted && (thumbWidth != static_cast<int>(origWidth) || thumbHeight != static_cast<int>(origHeight))) {
        hr = m_pFactory->CreateBitmapScaler(&pScaler);
        if (SUCCEEDED(hr) && pScaler) {
            hr = pScaler->Initialize(pConverted, thumbWidth, thumbHeight, WICBitmapInterpolationModeHighQualityCubic);
            if (SUCCEEDED(hr)) {
                pFinal = pScaler;
            } else {
                pScaler->Release();
                pScaler = nullptr;
            }
        }
    }

    if (pFinal) {
        result.hBitmap = CreateHBitmapFromWicBitmap(pFinal, thumbWidth, thumbHeight);
        result.success = (result.hBitmap != nullptr);
    }

    // Cleanup
    if (pScaler) pScaler->Release();
    if (pConverted && pConverted != pFrame) pConverted->Release();
    pFrame->Release();
    pDecoder->Release();

    return result;
}

bool ImageLoader::GetImageInfo(const std::wstring& filePath, int& width, int& height, std::wstring& format) {
    if (!m_initialized || filePath.empty()) return false;

    IWICBitmapDecoder* pDecoder = nullptr;
    HRESULT hr = m_pFactory->CreateDecoderFromFilename(
        filePath.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnDemand,
        &pDecoder
    );

    if (FAILED(hr) || !pDecoder) return false;

    GUID containerFormat;
    hr = pDecoder->GetContainerFormat(&containerFormat);
    if (SUCCEEDED(hr)) {
        if (containerFormat == GUID_ContainerFormatPng) format = L"PNG";
        else if (containerFormat == GUID_ContainerFormatJpeg) format = L"JPEG";
        else if (containerFormat == GUID_ContainerFormatGif) format = L"GIF";
        else if (containerFormat == GUID_ContainerFormatBmp) format = L"BMP";
        else if (containerFormat == GUID_ContainerFormatTiff) format = L"TIFF";
        else format = L"Image";
    }

    IWICBitmapFrameDecode* pFrame = nullptr;
    hr = pDecoder->GetFrame(0, &pFrame);
    if (SUCCEEDED(hr) && pFrame) {
        UINT w = 0, h = 0;
        pFrame->GetSize(&w, &h);
        width = static_cast<int>(w);
        height = static_cast<int>(h);
        pFrame->Release();
        pDecoder->Release();
        return true;
    }

    pDecoder->Release();
    return false;
}

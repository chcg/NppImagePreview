#include "MarginMarkerManager.h"
#include "Scintilla.h"

MarginMarkerManager::MarginMarkerManager() {}

MarginMarkerManager::~MarginMarkerManager() {
    Shutdown();
}

bool MarginMarkerManager::Initialize(HWND hSci) {
    if (!hSci || m_initialized) return false;

    m_hSci = hSci;
    SetupMarker();
    m_initialized = true;
    return true;
}

void MarginMarkerManager::Shutdown() {
    if (m_hSci && m_initialized) {
        ClearMarkers();
        m_hSci = nullptr;
    }
    m_initialized = false;
}

void MarginMarkerManager::SetupMarker() {
    if (!m_hSci) return;

    // Create a 16x16 image icon bitmap (RGBA)
    // Simple picture frame icon
    const int size = 16;
    const int bytesPerPixel = 4;
    const int stride = size * bytesPerPixel;

    unsigned char pixels[size * stride] = {};

    // Fill with transparent
    for (int i = 0; i < size * stride; i += 4) {
        pixels[i + 0] = 0;   // B
        pixels[i + 1] = 0;   // G
        pixels[i + 2] = 0;   // R
        pixels[i + 3] = 0;   // A
    }

    // Draw a simple image icon (picture frame with mountain)
    auto setPixel = [&](int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
        if (x < 0 || x >= size || y < 0 || y >= size) return;
        int idx = y * stride + x * bytesPerPixel;
        pixels[idx + 0] = b;
        pixels[idx + 1] = g;
        pixels[idx + 2] = r;
        pixels[idx + 3] = a;
    };

    // Frame border (light blue-gray)
    for (int x = 2; x < 14; x++) {
        setPixel(x, 2, 100, 180, 255, 255);
        setPixel(x, 13, 100, 180, 255, 255);
    }
    for (int y = 2; y < 14; y++) {
        setPixel(2, y, 100, 180, 255, 255);
        setPixel(13, y, 100, 180, 255, 255);
    }

    // Inner fill (slightly darker)
    for (int y = 3; y < 13; y++) {
        for (int x = 3; x < 13; x++) {
            setPixel(x, y, 60, 120, 180, 200);
        }
    }

    // Simple mountain/landscape shape
    for (int x = 4; x < 12; x++) {
        int h = 8 + (x > 7 ? (x - 7) : (7 - x));
        for (int y = h; y < 12; y++) {
            setPixel(x, y, 80, 160, 100, 220);
        }
    }

    // Sun/circle
    setPixel(10, 5, 255, 220, 80, 255);
    setPixel(11, 5, 255, 220, 80, 255);
    setPixel(10, 6, 255, 220, 80, 255);
    setPixel(11, 6, 255, 220, 80, 255);

    // Create bitmap from pixel data
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size;
    bmi.bmiHeader.biHeight = -size; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hDC = GetDC(nullptr);
    void* pBits = nullptr;
    HBITMAP hBmp = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
    if (hBmp && pBits) {
        memcpy(pBits, pixels, size * stride);
    }
    ReleaseDC(nullptr, hDC);

    // Register with Scintilla
    if (hBmp) {
        ::SendMessage(m_hSci, SCI_RGBAIMAGESETWIDTH, size, 0);
        ::SendMessage(m_hSci, SCI_RGBAIMAGESETHEIGHT, size, 0);
        ::SendMessage(m_hSci, SCI_RGBAIMAGESETSCALE, 100, 0);
        ::SendMessage(m_hSci, SCI_MARKERDEFINERGBAIMAGE, m_markerNumber, reinterpret_cast<LPARAM>(hBmp));
        DeleteObject(hBmp);
    }

    // Setup margin
    ::SendMessage(m_hSci, SCI_SETMARGINWIDTHN, 1, 16);
    ::SendMessage(m_hSci, SCI_SETMARGINTYPEN, 1, SC_MARGIN_SYMBOL);
    ::SendMessage(m_hSci, SCI_SETMARGINMASKN, 1, (1LL << m_markerNumber));
    ::SendMessage(m_hSci, SCI_SETMARGINSENSITIVEN, 1, 1);
}

void MarginMarkerManager::UpdateMarkers(const std::vector<ImageRef>& images) {
    if (!m_hSci) return;

    // Clear old markers
    ClearMarkers();

    // Set new markers
    for (const auto& img : images) {
        if (img.line >= 0) {
            ::SendMessage(m_hSci, SCI_MARKERADD, img.line, m_markerNumber);
            m_markedLines.insert(img.line);
        }
    }
}

void MarginMarkerManager::ClearMarkers() {
    if (!m_hSci) return;

    for (int line : m_markedLines) {
        ::SendMessage(m_hSci, SCI_MARKERDELETE, line, m_markerNumber);
    }
    m_markedLines.clear();
}

bool MarginMarkerManager::IsOnMarker(int line) {
    return m_markedLines.find(line) != m_markedLines.end();
}

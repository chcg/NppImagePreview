#pragma once

#include <Windows.h>
#include <string>
#include <memory>

struct PopupData {
    std::wstring fileName;
    std::wstring dimensions;
    std::wstring fileSize;
    std::wstring format;
    std::wstring fullPath;
    HBITMAP hThumbnail = nullptr;
    bool exists = true;
    bool isRemote = false;
};

class PopupWindow {
public:
    PopupWindow();
    ~PopupWindow();

    bool Create(HINSTANCE hInstance);
    void Destroy();

    void Show(const PopupData& data, int x, int y);
    void Hide();
    bool IsVisible() const;

    void UpdateThumbnail(HBITMAP hBitmap);

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void Paint(HDC hdc);
    void DrawThumbnail(HDC hdc, int x, int y, int maxWidth, int maxHeight);
    void DrawTextInfo(HDC hdc, int x, int y, int width);

    HWND m_hWnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    PopupData m_data;
    bool m_visible = false;

    // Layout constants
    static constexpr int POPUP_WIDTH = 280;
    static constexpr int THUMB_MAX_WIDTH = 240;
    static constexpr int THUMB_MAX_HEIGHT = 180;
    static constexpr int PADDING = 12;
    static constexpr int LINE_HEIGHT = 18;

    // Colors
    static constexpr COLORREF BG_COLOR = RGB(45, 45, 48);
    static constexpr COLORREF TEXT_COLOR = RGB(220, 220, 220);
    static constexpr COLORREF DIM_TEXT_COLOR = RGB(150, 150, 150);
    static constexpr COLORREF ACCENT_COLOR = RGB(0, 122, 204);
    static constexpr COLORREF ERROR_COLOR = RGB(220, 80, 80);
};

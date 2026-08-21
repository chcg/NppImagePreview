#define NOMINMAX
#include "PopupWindow.h"
#include <windowsx.h>
#include <algorithm>

static const wchar_t* POPUP_CLASS_NAME = L"NppImagePreviewPopup";

PopupWindow::PopupWindow() {}

PopupWindow::~PopupWindow() {
    Destroy();
}

bool PopupWindow::Create(HINSTANCE hInstance) {
    m_hInstance = hInstance;

    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = POPUP_CLASS_NAME;
    wcex.hbrBackground = CreateSolidBrush(BG_COLOR);

    if (!RegisterClassExW(&wcex)) {
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
    }

    int popupW = static_cast<int>(POPUP_WIDTH * 0.75);

    m_hWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        POPUP_CLASS_NAME,
        L"",
        WS_POPUP | WS_BORDER,
        0, 0, popupW, 200,
        nullptr, nullptr, hInstance, this
    );

    return m_hWnd != nullptr;
}

void PopupWindow::Destroy() {
    if (m_hWnd) {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
}

LRESULT CALLBACK PopupWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return 0;
    }

    PopupWindow* pThis = reinterpret_cast<PopupWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (pThis) {
        return pThis->HandleMessage(msg, wParam, lParam);
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT PopupWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(m_hWnd, &ps);
            Paint(hdc);
            EndPaint(m_hWnd, &ps);
            return 0;
        }
    }
    return DefWindowProc(m_hWnd, msg, wParam, lParam);
}

void PopupWindow::Show(const PopupData& data, int x, int y) {
    m_data = data;
    m_visible = true;

    int popupW = static_cast<int>(POPUP_WIDTH * 0.75);
    int thumbHeight = static_cast<int>((THUMB_MAX_HEIGHT / 2) * 1.15);

    int pad = static_cast<int>(PADDING * 0.6);
    int height = pad;

    if (data.hThumbnail && data.exists) {
        height += thumbHeight + 4;
    } else if (!data.exists) {
        height += 24;
    }

    height += LINE_HEIGHT * 2 + 2;
    height += pad;
    height = std::max(height, 60);

    HMONITOR hMonitor = MonitorFromPoint({x, y}, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(MONITORINFO) };
    GetMonitorInfo(hMonitor, &mi);

    int popupX = x + 20;
    int popupY = y + 20;

    if (popupX + popupW > mi.rcWork.right) {
        popupX = x - popupW - 10;
    }
    if (popupY + height > mi.rcWork.bottom) {
        popupY = y - height - 10;
    }

    SetWindowPos(m_hWnd, HWND_TOPMOST, popupX, popupY, popupW, height,
                 SWP_SHOWWINDOW | SWP_NOACTIVATE);

    InvalidateRect(m_hWnd, nullptr, TRUE);
    UpdateWindow(m_hWnd);
}

void PopupWindow::Hide() {
    m_visible = false;
    if (m_hWnd) {
        ShowWindow(m_hWnd, SW_HIDE);
    }
}

bool PopupWindow::IsVisible() const {
    return m_visible;
}

void PopupWindow::UpdateThumbnail(HBITMAP hBitmap) {
    m_data.hThumbnail = hBitmap;
    if (m_visible && m_hWnd) {
        InvalidateRect(m_hWnd, nullptr, TRUE);
        UpdateWindow(m_hWnd);
    }
}

void PopupWindow::Paint(HDC hdc) {
    RECT rc;
    GetClientRect(m_hWnd, &rc);

    HBRUSH hBgBrush = CreateSolidBrush(BG_COLOR);
    FillRect(hdc, &rc, hBgBrush);
    DeleteObject(hBgBrush);

    int pad = static_cast<int>(PADDING * 0.6);
    int y = pad;
    int x = pad;
    int contentWidth = rc.right - pad * 2;

    if (!m_data.exists) {
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        HFONT hFont = CreateFontW(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

        RECT textRc = { x, y, x + contentWidth, y + 18 };
        DrawTextW(hdc, L"Image not found", -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
        y += 22;
    } else if (m_data.hThumbnail) {
        int thumbHeight = static_cast<int>((THUMB_MAX_HEIGHT / 2) * 1.15);
        DrawThumbnail(hdc, x, y, contentWidth, thumbHeight);
        y += thumbHeight + 4;
    }

    DrawTextInfo(hdc, x, y, contentWidth);
}

void PopupWindow::DrawThumbnail(HDC hdc, int x, int y, int maxWidth, int maxHeight) {
    if (!m_data.hThumbnail) return;

    BITMAP bmp;
    GetObject(m_data.hThumbnail, sizeof(BITMAP), &bmp);

    int drawWidth = bmp.bmWidth;
    int drawHeight = bmp.bmHeight;

    if (drawWidth > maxWidth) {
        double ratio = static_cast<double>(maxWidth) / drawWidth;
        drawWidth = maxWidth;
        drawHeight = static_cast<int>(drawHeight * ratio);
    }
    if (drawHeight > maxHeight) {
        double ratio = static_cast<double>(maxHeight) / drawHeight;
        drawHeight = maxHeight;
        drawWidth = static_cast<int>(drawWidth * ratio);
    }

    // CENTER the thumbnail horizontally
    int offsetX = x + (maxWidth - drawWidth) / 2;

    HDC hMemDC = CreateCompatibleDC(hdc);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, m_data.hThumbnail);

    SetStretchBltMode(hdc, HALFTONE);
    StretchBlt(hdc, offsetX, y, drawWidth, drawHeight, hMemDC, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);

    SelectObject(hMemDC, hOldBmp);
    DeleteDC(hMemDC);
}

void PopupWindow::DrawTextInfo(HDC hdc, int x, int y, int width) {
    HFONT hFont = CreateFontW(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));  // white

    // CENTERED text
    RECT rc = { x, y, x + width, y + LINE_HEIGHT };
    DrawTextW(hdc, m_data.dimensions.c_str(), -1, &rc, DT_CENTER | DT_SINGLELINE);
    y += LINE_HEIGHT;

    rc = { x, y, x + width, y + LINE_HEIGHT };
    DrawTextW(hdc, m_data.fileSize.c_str(), -1, &rc, DT_CENTER | DT_SINGLELINE);

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}
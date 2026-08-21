#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>

#include "PluginInterface.h"
#include "Scintilla.h"
#include "Notepad_plus_msgs.h"
#include "ImageParser.h"
#include "PathResolver.h"
#include "ThumbnailCache.h"
#include "ImageLoader.h"
#include "PopupWindow.h"
#include "MarginMarkerManager.h"
//#include "ImagePanel.h"

// Plugin metadata
constexpr const wchar_t* PLUGIN_NAME = L"Image Preview";
constexpr int NB_FUNC = 5;

// Menu command IDs
enum MenuCmdId {
    CMD_TOGGLE_PREVIEW = 0,
    CMD_TOGGLE_PANEL = 1,
    CMD_CLEAR_CACHE = 2,
    CMD_PREVIEW_CURSOR = 3,
    CMD_ABOUT = 4
};

// Forward declarations
extern FuncItem funcItem[NB_FUNC];
extern NppData nppData;
extern bool g_isPreviewEnabled;

// Main plugin class
class ImagePreviewPlugin {
public:
    static ImagePreviewPlugin& Instance();

    bool Initialize(NppData nppData);
    void Shutdown();

    void OnBufferActivated();
    void OnDwellStart(int pos, int line);
    void OnDwellEnd();
    void OnModified(const SCNotification* notify);
    void OnUpdateUI();

    void TogglePreview();
    void TogglePanel();
    void ClearCache();
    void ShowAbout();
    void PreviewAtCursor();
    void ShowPopupForImage(const ImageRef& img);

    HWND GetCurrentScintilla();
    std::wstring GetCurrentFilePath();

private:
    ImagePreviewPlugin() = default;
    ~ImagePreviewPlugin() = default;
    ImagePreviewPlugin(const ImagePreviewPlugin&) = delete;
    ImagePreviewPlugin& operator=(const ImagePreviewPlugin&) = delete;

    void StartWorkerThreads();
    void StopWorkerThreads();
    void ProcessVisibleLines();

    NppData m_nppData{};
    HWND m_hScintilla = nullptr;

    std::unique_ptr<ImageParser> m_parser;
    std::unique_ptr<PathResolver> m_resolver;
    std::unique_ptr<ThumbnailCache> m_cache;
    std::unique_ptr<ImageLoader> m_loader;
    std::unique_ptr<PopupWindow> m_popup;
    std::unique_ptr<MarginMarkerManager> m_margins;
    // std::unique_ptr<ImagePanel> m_panel;  // Removed for v1, v2 feature

    // Worker threads
    std::atomic<bool> m_running{ false };
    std::thread m_parseThread;
    std::thread m_loadThread;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCv;
    std::queue<ImageRef> m_loadQueue;

    std::atomic<bool> m_needsReparse{ false };
    std::mutex m_visibleLinesMutex;
    std::vector<ImageRef> m_visibleImages;

    bool m_previewEnabled = true;
    bool m_panelVisible = false;
};

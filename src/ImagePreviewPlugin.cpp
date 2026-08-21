#include "ImagePreviewPlugin.h"
#include <thread>
#include <chrono>
#include <algorithm>

#ifndef SCI_GETCURRENTPOS
#define SCI_GETCURRENTPOS 2008
#endif
#ifndef SCI_LINEFROMPOSITION
#define SCI_LINEFROMPOSITION 2166
#endif
#ifndef SCI_GETFIRSTVISIBLELINE
#define SCI_GETFIRSTVISIBLELINE 2152
#endif
#ifndef SCI_SETMOUSEDWELLTIME
#define SCI_SETMOUSEDWELLTIME 2264
#endif

constexpr UINT MY_NPPMSG = WM_USER + 1000;
constexpr UINT MY_NPPM_GETCURRENTSCINTILLA = MY_NPPMSG + 4;
constexpr UINT MY_NPPM_GETFULLCURRENTPATH = MY_NPPMSG + 25;
constexpr UINT MY_NPPM_GETCURRENTBUFFERID = MY_NPPMSG + 60;
constexpr UINT MY_NPPM_GETFULLPATHFROMBUFFERID = MY_NPPMSG + 58;

FuncItem funcItem[NB_FUNC];
NppData nppData;
bool g_isPreviewEnabled = true;

static ImagePreviewPlugin* g_plugin = nullptr;

static void Log(const wchar_t* msg) {
    OutputDebugStringW(msg);
    OutputDebugStringW(L"\n");
    wchar_t tempPath[MAX_PATH];
    DWORD len = GetTempPathW(MAX_PATH, tempPath);
    if (len > 0 && len < MAX_PATH) {
        wcscat_s(tempPath, L"NppImagePreview_debug.log");
        FILE* f = nullptr;
        if (_wfopen_s(&f, tempPath, L"a, ccs=UTF-8") == 0 && f) {
            fwprintf(f, L"%s\n", msg);
            fclose(f);
        }
    }
}

ImagePreviewPlugin& ImagePreviewPlugin::Instance() {
    if (!g_plugin) g_plugin = new ImagePreviewPlugin();
    return *g_plugin;
}

bool ImagePreviewPlugin::Initialize(NppData data) {
    Log(L"Initialize called");
    m_nppData = data;

    m_previewEnabled = false;
    g_isPreviewEnabled = false;

    m_parser = std::make_unique<ImageParser>();
    m_resolver = std::make_unique<PathResolver>();
    m_cache = std::make_unique<ThumbnailCache>(100, 50);
    m_loader = std::make_unique<ImageLoader>();
    m_popup = std::make_unique<PopupWindow>();
    m_margins = std::make_unique<MarginMarkerManager>();

    HINSTANCE hInst = reinterpret_cast<HINSTANCE>(GetModuleHandleW(L"NppImagePreview.dll"));
    if (!hInst) {
        Log(L"GetModuleHandle failed!");
        hInst = GetModuleHandleW(nullptr);
    }

    if (!m_loader->Initialize()) {
        Log(L"ImageLoader init failed");
        return false;
    }

    if (!m_popup->Create(hInst)) {
        Log(L"PopupWindow create failed");
        return false;
    }
    Log(L"PopupWindow created OK");

    if (m_nppData._scintillaMainHandle) {
        ::SendMessage(m_nppData._scintillaMainHandle, SCI_SETMOUSEDWELLTIME, 400, 0);
        Log(L"Dwell time set on main editor");
    }
    if (m_nppData._scintillaSecondHandle) {
        ::SendMessage(m_nppData._scintillaSecondHandle, SCI_SETMOUSEDWELLTIME, 400, 0);
        Log(L"Dwell time set on second editor");
    }

    StartWorkerThreads();
    return true;
}

void ImagePreviewPlugin::Shutdown() {
    StopWorkerThreads();
    if (m_popup) m_popup->Destroy();
    if (m_margins) m_margins->Shutdown();
    if (m_loader) m_loader->Shutdown();
    if (m_cache) m_cache->Clear();
}

void ImagePreviewPlugin::StartWorkerThreads() {
    m_running = true;

    m_parseThread = std::thread([this]() {
        while (m_running) {
            if (m_needsReparse.load()) {
                m_needsReparse = false;
                ProcessVisibleLines();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    m_loadThread = std::thread([this]() {
        while (m_running) {
            ImageRef ref;
            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_queueCv.wait(lock, [this]() { return !m_loadQueue.empty() || !m_running; });
                if (!m_running) break;
                if (m_loadQueue.empty()) continue;
                ref = m_loadQueue.front();
                m_loadQueue.pop();
            }

            if (!ref.resolvedPath.empty() && !m_cache->Has(ref.resolvedPath)) {
                auto loaded = m_loader->LoadThumbnail(ref.resolvedPath, 200);
                if (loaded.success && loaded.hBitmap) {
                    WIN32_FILE_ATTRIBUTE_DATA fad;
                    FILETIME ft = {};
                    uint64_t fileSize = 0;
                    if (GetFileAttributesExW(ref.resolvedPath.c_str(), GetFileExInfoStandard, &fad)) {
                        ft = fad.ftLastWriteTime;
                        fileSize = (static_cast<uint64_t>(fad.nFileSizeHigh) << 32) | fad.nFileSizeLow;
                    }
                    m_cache->Put(ref.resolvedPath, loaded.hBitmap, loaded.width, loaded.height, fileSize, ft);
                }
            }
        }
    });
}

void ImagePreviewPlugin::StopWorkerThreads() {
    m_running = false;
    m_queueCv.notify_all();
    if (m_parseThread.joinable()) m_parseThread.join();
    if (m_loadThread.joinable()) m_loadThread.join();
}

void ImagePreviewPlugin::ProcessVisibleLines() {
    HWND hSci = GetCurrentScintilla();
    if (!hSci) {
        Log(L"ProcessVisibleLines: no scintilla handle");
        return;
    }

    std::wstring currentFile = GetCurrentFilePath();
    if (currentFile.empty())
        Log(L"ProcessVisibleLines: currentFile is EMPTY - will parse but not resolve paths");
    else
        Log(currentFile.c_str());

    auto images = m_parser->ParseVisibleLines(hSci, currentFile);

    wchar_t buf[256];
    swprintf_s(buf, L"ProcessVisibleLines: found %zu image(s)", images.size());
    Log(buf);

    for (auto& img : images) {
        if (!currentFile.empty()) {
            img.resolvedPath = m_resolver->ResolvePath(img.rawPath, currentFile);
            img.exists = PathResolver::FileExists(img.resolvedPath);
        } else {
            img.resolvedPath = img.rawPath;
            img.exists = false;
        }
        img.isRemote = PathResolver::IsRemoteUrl(img.rawPath);

        if (img.exists && !img.isRemote && !m_cache->Has(img.resolvedPath)) {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (m_loadQueue.size() < 20) {
                m_loadQueue.push(img);
                m_queueCv.notify_one();
            }
        }
    }

    if (m_margins && m_previewEnabled && !currentFile.empty()) {
        if (!m_margins->IsInitialized()) m_margins->Initialize(hSci);
        m_margins->UpdateMarkers(images);
    }

    {
        std::lock_guard<std::mutex> lock(m_visibleLinesMutex);
        m_visibleImages = images;
    }
}

HWND ImagePreviewPlugin::GetCurrentScintilla() {
    int currentEdit = 0;
    ::SendMessage(m_nppData._nppHandle, MY_NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&currentEdit));
    return (currentEdit == 0) ? m_nppData._scintillaMainHandle : m_nppData._scintillaSecondHandle;
}

std::wstring ImagePreviewPlugin::GetCurrentFilePath() {
    wchar_t path[MAX_PATH] = {};
    ::SendMessage(m_nppData._nppHandle, MY_NPPM_GETFULLCURRENTPATH, 0, reinterpret_cast<LPARAM>(path));
    if (path[0] != L'\0') { Log(path); return path; }

    Log(L"GetCurrentFilePath: NPPM_GETFULLCURRENTPATH returned empty, trying buffer ID fallback");
    LRESULT bufferID = ::SendMessage(m_nppData._nppHandle, MY_NPPM_GETCURRENTBUFFERID, 0, 0);
    if (bufferID != 0) {
        ::SendMessage(m_nppData._nppHandle, MY_NPPM_GETFULLPATHFROMBUFFERID,
                       static_cast<WPARAM>(bufferID), reinterpret_cast<LPARAM>(path));
        if (path[0] != L'\0') { Log(path); return path; }
    }

    Log(L"GetCurrentFilePath: ALL METHODS FAILED");
    return L"";
}

void ImagePreviewPlugin::OnBufferActivated() {
    HWND hSci = GetCurrentScintilla();
    if (hSci) ::SendMessage(hSci, SCI_SETMOUSEDWELLTIME, 400, 0);
    m_needsReparse = true;
}

void ImagePreviewPlugin::OnDwellStart(int pos, int line) {
    Log(L"OnDwellStart called");
    if (!m_previewEnabled || !m_popup) return;

    HWND hSci = GetCurrentScintilla();
    if (!hSci) { Log(L"No scintilla handle"); return; }

    int actualLine = static_cast<int>(::SendMessage(hSci, SCI_LINEFROMPOSITION, pos, 0));

    std::vector<ImageRef> images;
    {
        std::lock_guard<std::mutex> lock(m_visibleLinesMutex);
        images = m_visibleImages;
    }

    if (images.empty()) {
        Log(L"OnDwellStart: m_visibleImages empty, forcing sync parse...");
        ProcessVisibleLines();
        {
            std::lock_guard<std::mutex> lock(m_visibleLinesMutex);
            images = m_visibleImages;
        }
        wchar_t buf[256];
        swprintf_s(buf, L"OnDwellStart: after sync parse, found %zu image(s)", images.size());
        Log(buf);
    }

    if (images.empty()) { Log(L"No visible images found"); return; }

    const ImageRef* match = nullptr;
    for (const auto& img : images) {
        if (img.line == actualLine) { match = &img; break; }
    }

    if (!match) { Log(L"No image on this line"); return; }
    ShowPopupForImage(*match);
}

void ImagePreviewPlugin::ShowPopupForImage(const ImageRef& img) {
    if (!m_popup) return;

    POINT pt;
    GetCursorPos(&pt);

    PopupData data;
    data.fileName = img.rawPath;
    size_t lastSlash = data.fileName.find_last_of(L"/\\");
    if (lastSlash != std::wstring::npos)
        data.fileName = data.fileName.substr(lastSlash + 1);

    data.fullPath = img.resolvedPath.empty() ? img.rawPath : img.resolvedPath;
    data.exists = img.exists;
    data.isRemote = img.isRemote;

    if (img.exists && !img.isRemote) {
        auto dims = PathResolver::GetImageDimensions(img.resolvedPath);
        std::wstring sizeStr = PathResolver::GetFileSizeString(img.resolvedPath);
        std::wstring dimStr = std::to_wstring(dims.first) + L" x " + std::to_wstring(dims.second) + L" px";

        int w, h;
        std::wstring format;
        m_loader->GetImageInfo(img.resolvedPath, w, h, format);

        data.dimensions = dimStr;
        data.fileSize = sizeStr;
        data.format = format;

        auto cached = m_cache->Get(img.resolvedPath);
        if (cached) data.hThumbnail = cached->hBitmap;
    } else if (img.isRemote) {
        data.dimensions = L"Remote URL";
        data.fileSize = L"Unknown";
        data.format = L"Web Image";
    } else {
        data.dimensions = L"File not found / Unsaved buffer";
        data.fileSize = L"N/A";
        data.format = L"Unknown";
    }

    Log(L"Showing popup");
    m_popup->Show(data, pt.x, pt.y);
}

void ImagePreviewPlugin::PreviewAtCursor() {
    Log(L"PreviewAtCursor called");

    if (!m_popup) {
        MessageBoxW(nullptr, L"m_popup is null!", L"Debug", MB_OK);
        return;
    }

    HWND hSci = GetCurrentScintilla();
    if (!hSci) {
        MessageBoxW(nullptr, L"No scintilla handle!", L"Debug", MB_OK);
        Log(L"No scintilla handle for PreviewAtCursor");
        return;
    }

    int pos = static_cast<int>(::SendMessage(hSci, SCI_GETCURRENTPOS, 0, 0));
    int line = static_cast<int>(::SendMessage(hSci, SCI_LINEFROMPOSITION, pos, 0));

    std::vector<ImageRef> images;
    {
        std::lock_guard<std::mutex> lock(m_visibleLinesMutex);
        images = m_visibleImages;
    }

    if (images.empty()) {
        m_needsReparse = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        {
            std::lock_guard<std::mutex> lock(m_visibleLinesMutex);
            images = m_visibleImages;
        }
    }

    const ImageRef* match = nullptr;
    for (const auto& img : images) {
        if (img.line == line) { match = &img; break; }
    }

    if (!match && !images.empty()) match = &images[0];

    if (match) {
        ShowPopupForImage(*match);
    } else {
        Log(L"No image found for PreviewAtCursor");
        MessageBoxW(nullptr, L"No image found. Try saving the file first.", L"Image Preview", MB_OK | MB_ICONINFORMATION);
    }
}

void ImagePreviewPlugin::OnDwellEnd() {
    if (m_popup) m_popup->Hide();
}

void ImagePreviewPlugin::OnModified(const SCNotification* notify) {
    m_needsReparse = true;
}

void ImagePreviewPlugin::OnUpdateUI() {
    static int lastFirstLine = -1;
    HWND hSci = GetCurrentScintilla();
    if (!hSci) return;

    int firstVisible = static_cast<int>(::SendMessage(hSci, SCI_GETFIRSTVISIBLELINE, 0, 0));
    if (abs(firstVisible - lastFirstLine) > 2) {
        lastFirstLine = firstVisible;
        m_needsReparse = true;
    }
}

void ImagePreviewPlugin::TogglePreview() {
    m_previewEnabled = !m_previewEnabled;
    g_isPreviewEnabled = m_previewEnabled;

    HMENU hMenu = GetMenu(m_nppData._nppHandle);
    if (hMenu) {
        int count = GetMenuItemCount(hMenu);
        for (int i = 0; i < count; i++) {
            wchar_t name[256] = {};
            GetMenuStringW(hMenu, i, name, 256, MF_BYPOSITION);
            if (wcsstr(name, L"Plugins") || wcsstr(name, L"plugins") || wcsstr(name, L"Plugin")) {
                HMENU hPlugins = GetSubMenu(hMenu, i);
                if (hPlugins) {
                    int pCount = GetMenuItemCount(hPlugins);
                    for (int j = 0; j < pCount; j++) {
                        wchar_t pName[256] = {};
                        GetMenuStringW(hPlugins, j, pName, 256, MF_BYPOSITION);
                        if (wcsstr(pName, L"Image Preview")) {
                            HMENU hOurMenu = GetSubMenu(hPlugins, j);
                            if (hOurMenu) {
                                int ourCount = GetMenuItemCount(hOurMenu);
                                for (int k = 0; k < ourCount; k++) {
                                    wchar_t itemName[256] = {};
                                    GetMenuStringW(hOurMenu, k, itemName, 256, MF_BYPOSITION);
                                    if (wcsstr(itemName, L"Enable Image Preview")) {
                                        UINT check = m_previewEnabled ? MF_CHECKED : MF_UNCHECKED;
                                        CheckMenuItem(hOurMenu, k, check | MF_BYPOSITION);
                                        DrawMenuBar(m_nppData._nppHandle);
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                    }
                }
                break;
            }
        }
    }

    if (!m_previewEnabled) {
        if (m_popup) m_popup->Hide();
        if (m_margins) m_margins->ClearMarkers();
    } else {
        m_needsReparse = true;
    }
}

void ImagePreviewPlugin::TogglePanel() {
    MessageBoxW(m_nppData._nppHandle,
        L"Image Panel is disabled in this version.\n\n"
        L"Use hover over image paths, or click Preview Image at Cursor.",
        L"Image Preview",
        MB_OK | MB_ICONINFORMATION);
}

void ImagePreviewPlugin::ClearCache() {
    if (m_cache) m_cache->Clear();
    Log(L"Thumbnail cache cleared");
}

void ImagePreviewPlugin::ShowAbout() {
    MessageBoxW(m_nppData._nppHandle,
        L"NppImagePreview\n\n"
		L"Notepad++ Image Preview Plugin v1.0\n\n"
        L"Author: Mashkawat Ahsan\n"
        L"GitHub: https://github.com/bdgeek\n\n"
        L"Hover over image paths in HTML, CSS, PHP, or JS files\n"
        L"to see a thumbnail preview popup.\n\n"
        L"Use Preview Image at Cursor to manually trigger the popup.\n\n"
        L"Supported formats: PNG, JPG, WebP, GIF, SVG, BMP, ICO, AVIF, TIFF",
        L"About Image Preview",
        MB_OK | MB_ICONINFORMATION);
}

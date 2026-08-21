// Fallback defines for older/missing Scintilla headers
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

#include "ImagePreviewPlugin.h"
#include "menuCmdID.h"
#include <Windows.h>

// Simple debug logger using Windows API
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

// Notepad++ required exports
extern "C" __declspec(dllexport) void setInfo(NppData notepadPlusData) {
    nppData = notepadPlusData;
}

extern "C" __declspec(dllexport) const wchar_t* getName() {
    return PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem* getFuncsArray(int* nbF) {
    *nbF = NB_FUNC;
    return funcItem;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification* notify) {
    auto& plugin = ImagePreviewPlugin::Instance();

    switch (notify->nmhdr.code) {
        case NPPN_READY:
            Log(L"NPPN_READY received");
            plugin.Initialize(nppData);
            ::SendMessage(nppData._scintillaMainHandle, SCI_SETMOUSEDWELLTIME, 500, 0);
            ::SendMessage(nppData._scintillaSecondHandle, SCI_SETMOUSEDWELLTIME, 500, 0);
            plugin.OnBufferActivated();
            break;

        case NPPN_BUFFERACTIVATED:
        case NPPN_LANGCHANGED:
            ::SendMessage(nppData._scintillaMainHandle, SCI_SETMOUSEDWELLTIME, 500, 0);
            ::SendMessage(nppData._scintillaSecondHandle, SCI_SETMOUSEDWELLTIME, 500, 0);
            plugin.OnBufferActivated();
            break;

        case NPPN_SHUTDOWN:
            plugin.Shutdown();
            break;

        case SCN_DWELLSTART:
            Log(L"SCN_DWELLSTART received");
            if (notify->position >= 0) {
                plugin.OnDwellStart(static_cast<int>(notify->position), 
                                   static_cast<int>(notify->line));
            }
            break;

        case SCN_DWELLEND:
            plugin.OnDwellEnd();
            break;

        case SCN_MODIFIED:
            plugin.OnModified(notify);
            break;

        case SCN_UPDATEUI:
            if (notify->updated & (SC_UPDATE_V_SCROLL | SC_UPDATE_H_SCROLL | SC_UPDATE_CONTENT)) {
                plugin.OnUpdateUI();
            }
            break;
    }
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT msg, WPARAM wParam, LPARAM lParam) {
    return TRUE;
}

extern "C" __declspec(dllexport) BOOL isUnicode() {
    return TRUE;
}

// Menu command handlers
static void togglePreview() {
    ImagePreviewPlugin::Instance().TogglePreview();
}

static void togglePanel() {
    ImagePreviewPlugin::Instance().TogglePanel();
}

static void clearCache() {
    ImagePreviewPlugin::Instance().ClearCache();
}

static void showAbout() {
    ImagePreviewPlugin::Instance().ShowAbout();
}

static void previewAtCursor() {
    ImagePreviewPlugin::Instance().PreviewAtCursor();
}

BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD reason, LPVOID lpvReserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            wcscpy_s(funcItem[CMD_TOGGLE_PREVIEW]._itemName, L"Enable Image Preview");
            funcItem[CMD_TOGGLE_PREVIEW]._pFunc = togglePreview;
            funcItem[CMD_TOGGLE_PREVIEW]._init2Check = false;
            funcItem[CMD_TOGGLE_PREVIEW]._pShKey = nullptr;

            wcscpy_s(funcItem[CMD_TOGGLE_PANEL]._itemName, L"Show Image Panel");
            funcItem[CMD_TOGGLE_PANEL]._pFunc = togglePanel;
            funcItem[CMD_TOGGLE_PANEL]._init2Check = false;
            funcItem[CMD_TOGGLE_PANEL]._pShKey = nullptr;

            wcscpy_s(funcItem[CMD_CLEAR_CACHE]._itemName, L"Clear Thumbnail Cache");
            funcItem[CMD_CLEAR_CACHE]._pFunc = clearCache;
            funcItem[CMD_CLEAR_CACHE]._init2Check = false;
            funcItem[CMD_CLEAR_CACHE]._pShKey = nullptr;

            wcscpy_s(funcItem[CMD_PREVIEW_CURSOR]._itemName, L"Preview Image at Cursor");
            funcItem[CMD_PREVIEW_CURSOR]._pFunc = previewAtCursor;
            funcItem[CMD_PREVIEW_CURSOR]._init2Check = false;
            funcItem[CMD_PREVIEW_CURSOR]._pShKey = nullptr;

            wcscpy_s(funcItem[CMD_ABOUT]._itemName, L"About");
            funcItem[CMD_ABOUT]._pFunc = showAbout;
            funcItem[CMD_ABOUT]._init2Check = false;
            funcItem[CMD_ABOUT]._pShKey = nullptr;
            break;
    }
    return TRUE;
}

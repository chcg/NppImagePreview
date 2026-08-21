#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include <windows.h>
#include "Sci_Position.h"

const int nbChar = 64;

typedef void (__cdecl * PFUNCPLUGINCMD)();

typedef struct _ShortcutKey {
	BOOL _isCtrl;
	BOOL _isAlt;
	BOOL _isShift;
	UCHAR _key;
} ShortcutKey;

typedef struct _FuncItem {
	wchar_t _itemName[nbChar];
	PFUNCPLUGINCMD _pFunc;
	int _cmdID;
	BOOL _init2Check;
	ShortcutKey *_pShKey;
} FuncItem;

typedef const wchar_t * (__cdecl * PFUNCGETNAME)();

typedef struct _NppData {
	HWND _nppHandle;
	HWND _scintillaMainHandle;
	HWND _scintillaSecondHandle;
} NppData;

typedef enum {
	notepadPlusReady = 1,
	NPPN_TBMODIFICATION = 2048,
	NPPN_READY = 1001,
	NPPN_SHUTDOWN = 1009,
	NPPN_BUFFERACTIVATED = 1010,
	NPPN_LANGCHANGED = 1011
} NppNotif;

typedef struct {
	HWND hwndFrom;
	UINT idFrom;
	UINT code;
} SCNotificationHeader;

// CRITICAL: Must match Scintilla's actual struct layout for x64
// Sci_Position is ptrdiff_t (8 bytes on x64, 4 bytes on x86)
typedef struct {
	NMHDR nmhdr;
	Sci_Position position;       // was int - WRONG on x64
	int ch;
	int modifiers;
	int modificationType;
	const char *text;
	Sci_Position length;         // was int - WRONG on x64
	Sci_Position linesAdded;     // was int - WRONG on x64
	int message;
	UINT_PTR wParam;
	INT_PTR lParam;
	Sci_Position line;           // was int - WRONG on x64
	int foldLevelNow;
	int foldLevelPrev;
	int margin;
	int listType;
	int x;
	int y;
	int token;
	int annotationLinesAdded;
	int updated;
	int listCompletionMethod;
} SCNotification;

#endif //PLUGININTERFACE_H
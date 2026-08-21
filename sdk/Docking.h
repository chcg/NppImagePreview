#ifndef DOCKING_H
#define DOCKING_H

#include <windows.h>

#define CONT_LEFT 0
#define CONT_RIGHT 1
#define CONT_TOP 2
#define CONT_BOTTOM 3
#define CONT_FLOATING 4

#define DOCKCONT_MAX 4

#define DWS_DF_CONT_LEFT (CONT_LEFT << 28)
#define DWS_DF_CONT_RIGHT (CONT_RIGHT << 28)
#define DWS_DF_CONT_TOP (CONT_TOP << 28)
#define DWS_DF_CONT_BOTTOM (CONT_BOTTOM << 28)
#define DWS_DF_FLOATING CONT_FLOATING

#define DWS_ICONTAB 0x00000001
#define DWS_ICONBAR 0x00000002
#define DWS_ADDINFO 0x00000004
#define DWS_PARAMSALL (DWS_ICONTAB | DWS_ICONBAR | DWS_ADDINFO)

#define DMN_FIRST 1050
#define DMN_CLOSE (DMN_FIRST + 1)
#define DMN_DOCK (DMN_FIRST + 2)
#define DMN_FLOAT (DMN_FIRST + 3)
#define DMN_SWITCHIN (DMN_FIRST + 4)
#define DMN_SWITCHOFF (DMN_FIRST + 5)
#define DMN_FLOATDROPPED (DMN_FIRST + 6)

typedef struct {
	HWND hClient;
	const wchar_t *pszName;
	int dlgID;
	UINT uMask;
	HICON hIconTab;
	const wchar_t *pszAddInfo;
	RECT rcFloat;
	int iPrevCont;
	const wchar_t *pszModuleName;
} tTbData;

typedef struct {
	HWND hWnd;
	UINT uMsg;
	WPARAM wParam;
	LPARAM lParam;
} tDockMgr;

#endif // DOCKING_H
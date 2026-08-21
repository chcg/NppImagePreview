#ifndef STATIC_DIALOG_H
#define STATIC_DIALOG_H

#include <windows.h>
#include "Window.h"

enum PosAlign { ALIGNPOS_LEFT, ALIGNPOS_RIGHT, ALIGNPOS_TOP, ALIGNPOS_BOTTOM };

class StaticDialog : public Window {
public:
	StaticDialog() : Window() {}
	~StaticDialog() {}

	void goToCenter() {}

	void display(bool toShow = true) const {
		if (_hSelf) ::ShowWindow(_hSelf, toShow ? SW_SHOW : SW_HIDE);
	}

	void create(int dialogID, bool isRTL = false) {
		_hSelf = ::CreateDialogParam(_hInst, MAKEINTRESOURCE(dialogID), _hParent, dlgProc, reinterpret_cast<LPARAM>(this));
		if (_hSelf) ::ShowWindow(_hSelf, SW_HIDE); // Create hidden, docking system will show it
	}

	HWND getHSelf() const { return _hSelf; }

protected:
	RECT _rc;

	static INT_PTR CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
		if (message == WM_INITDIALOG) {
			StaticDialog* pDlg = reinterpret_cast<StaticDialog*>(lParam);
			pDlg->_hSelf = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, static_cast<LONG_PTR>(lParam));
			return pDlg->run_dlgProc(message, wParam, lParam);
		}
		StaticDialog* pDlg = reinterpret_cast<StaticDialog*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if (pDlg) {
			return pDlg->run_dlgProc(message, wParam, lParam);
		}
		return FALSE;
	}

	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) = 0;

	void alignWith(HWND handle, HWND handle2Align, PosAlign pos, POINT& point) {}
};

#endif // STATIC_DIALOG_H
#ifndef WINDOW_H
#define WINDOW_H

#include <windows.h>

class Window {
public:
	Window() : _hInst(NULL), _hParent(NULL), _hSelf(NULL) {}
	virtual ~Window() {}

	virtual void init(HINSTANCE hInst, HWND parent) {
		_hInst = hInst;
		_hParent = parent;
	}

	virtual void destroy() {}

	virtual void display(bool toShow = true) const {
		::ShowWindow(_hSelf, toShow ? SW_SHOW : SW_HIDE);
	}

	virtual void reSizeTo(RECT& rc) {
		::MoveWindow(_hSelf, rc.left, rc.top, rc.right, rc.bottom, TRUE);
	}

	virtual HWND getHSelf() const { return _hSelf; }
	virtual HINSTANCE getHinst() const { return _hInst; }

protected:
	HINSTANCE _hInst;
	HWND _hParent;
	HWND _hSelf;
};

#endif // WINDOW_H
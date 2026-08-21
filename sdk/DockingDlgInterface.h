#ifndef DOCKINGDLGINTERFACE_H
#define DOCKINGDLGINTERFACE_H

#include <windows.h>
#include <string>
#include "StaticDialog.h"
#include "Docking.h"

class DockingDlgInterface : public StaticDialog {
public:
	DockingDlgInterface(int dlgID) : StaticDialog(), _dlgID(dlgID) {}
	~DockingDlgInterface() {}

	void create(tTbData* data, bool isRTL = false) {
		StaticDialog::create(_dlgID, isRTL);
		::GetClientRect(_hSelf, &_rc);

		data->hClient = _hSelf;
		data->pszName = _pluginName.c_str();
		data->dlgID = _dlgID;
		data->uMask = _mask;
		data->hIconTab = _hIcon;
		data->pszAddInfo = _additionalInfo.c_str();
		data->rcFloat = _rc;
		data->iPrevCont = 0;
		data->pszModuleName = getPluginFileName();
	}

	void init(HINSTANCE hInst, HWND parent) {
		Window::init(hInst, parent);
	}

	virtual void setParent(HWND parent2set) {
		_hParent = parent2set;
		::SetParent(_hSelf, parent2set);
	}

	void destroy() {
		::DestroyWindow(_hSelf);
	}

protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) = 0;

	int _dlgID = 0;
	bool _isClosing = false;
	UINT _mask = 0;
	HICON _hIcon = NULL;
	std::wstring _pluginName;
	std::wstring _additionalInfo;

	virtual const wchar_t* getPluginFileName() const { return L"NppImagePreview.dll"; }
};

#endif // DOCKINGDLGINTERFACE_H
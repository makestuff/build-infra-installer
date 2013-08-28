/* rc -nologo resource.rc; \
 * cl -O1 -Os -EHsc -D UNICODE -D _UNICODE -D _WIN32_IE=0x0500 -D WINVER=0x600 -I. -Femain *.cpp \
 * kernel32.lib user32.lib ole32.lib shell32.lib advapi32.lib comctl32.lib resource.res
 */
#include <string>
#include <vector>
#include <iterator>
#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include "exception.h"
#include "registry.h"
#include "shellLink.h"
#include "installer.h"
#include "util.h"

using namespace std;
	
// -------------------------------------------------------------------------------------------------
// Callback function that is invoked whenever the user does something.
//
INT_PTR CALLBACK dialogCallback(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static Installer *installer = NULL;
	if ( uMsg == WM_COMMAND ) {
		if ( HIWORD(wParam) == BN_CLICKED ) {
			switch ( LOWORD(wParam) ) {
			case IDCANCEL:
			case IDOK:
			case IDC_REFRESH:
				EndDialog(hwndDlg, (INT_PTR)LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
		} else if ( HIWORD(wParam) == CBN_SELCHANGE ) {
			const DWORD selection = SendMessage((HWND)lParam, CB_GETCURSEL, NULL, NULL);
			const DWORD dropdown = LOWORD(wParam);
			const HWND info = GetDlgItem(hwndDlg, dropdown+1);
			const HWND linkName = GetDlgItem(hwndDlg, IDC_LINKNAME);
			installer->selectionChanged(dropdown, selection);
			SendMessage(linkName, WM_SETTEXT, 0, (LPARAM)installer->getLinkName());
			if ( dropdown == IDC_PYTHONLIST ) {
				const vector<pair<wstring, wstring>> &pyInstalls = installer->getPythonList();
				const pair<wstring, wstring> &pyChoice = pyInstalls[selection];
				const wstring path = pyChoice.second + L"Lib\\site-packages\\yaml";
				MessageBox(hwndDlg, path.c_str(), L"Searching", MB_OK|MB_ICONINFORMATION);
				if ( !isFilePresent(path) ) {
					const wstring &version = pyChoice.first;
					installer->hdlUseable(false);
					if ( version == L"2.7" || (version[0] == L'3' && version[2] >= L'0' && version[2] <= L'2') ) {
						wstring message =
							L"To use hdlmake.py, you need PyYAML installed. Install it from "
							L"<a href=\"http://pyyaml.org/download/pyyaml/PyYAML-3.10.win32-py";
						message += version;
						message += L".exe\">here</a>.";
						SendMessage(
							info, WM_SETTEXT, 0, (LPARAM)message.c_str()
						);
					} else {
						wstring message =
							L"To use hdlmake.py, you need "
							L"<a href=\"http://pyyaml.org\">PyYAML</a> installed. Unfortunately, "
							L"there is no pre-built installer for Python ";
						message += version;
						message +=
							L". You could try installing it yourself some other way, but the "
							L"easiest solution is to just choose a different Python version.";
						SendMessage(
							info, WM_SETTEXT, 0, (LPARAM)message.c_str()
						);
					}
				} else {
					installer->hdlUseable(true);
				}
			}
		}
	} else if ( uMsg == WM_NOTIFY ) {
		const NMLINK *link = (const NMLINK *)lParam;
		switch ( link->hdr.code ) {
			case NM_CLICK:
			case NM_RETURN: {
				const LITEM *item = &link->item;
				//wostringstream os;
				//os << L"Clicked on link[" << item->iLink << L"]: " << item->szUrl;
				//MessageBox(hwndDlg, os.str().c_str(), L"URL click", MB_OK|MB_ICONINFORMATION);
				ShellExecute(NULL, L"open", item->szUrl, NULL, NULL, SW_SHOW);
				break;
			}
		}
	} else if ( uMsg == WM_INITDIALOG ) {
		// Initialise installer (static)
		installer = (Installer *)lParam;
		const vector<VSInstall> &vsInstalls = installer->getCompilerList();
		const vector<pair<wstring, wstring>> &pyInstalls = installer->getPythonList();
		const vector<pair<wstring, wstring>> &iseInstalls = installer->getXilinxList();
		const HWND vsList = GetDlgItem(hwndDlg, IDC_COMPILERLIST);
		const HWND vsInfo = GetDlgItem(hwndDlg, IDC_COMPILERINFO);
		const HWND pyList = GetDlgItem(hwndDlg, IDC_PYTHONLIST);
		const HWND pyInfo = GetDlgItem(hwndDlg, IDC_PYTHONINFO);
		const HWND iseList = GetDlgItem(hwndDlg, IDC_XILINXLIST);
		const HWND linkName = GetDlgItem(hwndDlg, IDC_LINKNAME);
		SendMessage(linkName, WM_SETTEXT, 0, (LPARAM)installer->getLinkName());

		// Reset combo-boxes
		//SendMessage(vsList, CB_RESETCONTENT, 0, 0);
		//SendMessage(pyList, CB_RESETCONTENT, 0, 0);

		// Populate C/C++ Compilers list
		if ( vsInstalls.empty() ) {
			//ShowWindow(child, SW_SHOW);
			SendMessage(
				vsInfo, WM_SETTEXT, 0, (LPARAM)
				L"You have no MSVC compilers installed. Install Visual Studio "
				L"<a href=\"http://go.microsoft.com/?linkid=7729279\">2008</a>, "
				L"<a href=\"http://go.microsoft.com/?linkid=9709949\">2010</a>, "
				L"<a href=\"http://go.microsoft.com/?linkid=9816758\">2012</a> or "
				L"<a href=\"http://go.microsoft.com/?linkid=9833050\">2013</a>. "
				L"Alternatively, if you don't need the IDE, just install "
				L"<a href=\"http://download.microsoft.com/download/A/6/A/A6AC035D-DA3F-4F0C-ADA4-37C8E5D34E3D/winsdk_web.exe\">SDK 7.1</a>."
			);
		} else {
			//ShowWindow(child, SW_HIDE);
			SendMessage(vsInfo, WM_SETTEXT, 0, (LPARAM)L"Select the MSVC compiler you wish to use.");
			for ( vector<VSInstall>::const_iterator i = vsInstalls.begin(); i != vsInstalls.end(); i++ ) {
				SendMessage(vsList, CB_ADDSTRING, 0, (LPARAM)i->name.c_str());
			}
		}

		// Populate Python Interpreters list
		const bool x64 = is64();
		if ( pyInstalls.empty() ) {
			//ShowWindow(child, SW_SHOW);
			if ( x64 ) {
				SendMessage(
					pyInfo, WM_SETTEXT, 0, (LPARAM)
					L"No Python interpreters installed. Get Python 2.7 ("
					L"<a href=\"http://www.python.org/ftp/python/2.7.5/python-2.7.5.amd64.msi\">x64</a> or "
					L"<a href=\"http://www.python.org/ftp/python/2.7.5/python-2.7.5.msi\">x86</a>), or 3.2 ("
					L"<a href=\"http://www.python.org/ftp/python/3.2.5/python-3.2.5.amd64.msi\">x64</a> or "
					L"<a href=\"http://www.python.org/ftp/python/3.2.5/python-3.2.5.msi\">x86</a>). "
					L"Install \"for just me\" rather than \"for all users\" to avoid "
					L"<a href=\"http://stackoverflow.com/a/7170483/2694208\">this PyYAML installer bug</a>."
				);
			} else {
				SendMessage(
					pyInfo, WM_SETTEXT, 0, (LPARAM)
					L"You have no Python interpreters installed. Install "
					L"<a href=\"http://www.python.org/ftp/python/2.7.5/python-2.7.5.msi\">Python 2.7</a> or "
					L"<a href=\"http://www.python.org/ftp/python/3.2.5/python-3.2.5.msi\">Python 3.2</a>."
				);
			}
		} else {
			//ShowWindow(child, SW_HIDE);
			const wstring baseItem = L"Python ";
			SendMessage(pyInfo, WM_SETTEXT, 0, (LPARAM)L"Select the Python interpreter you wish to use.");
			for ( vector<pair<wstring, wstring>>::const_iterator i = pyInstalls.begin(); i != pyInstalls.end(); i++ ) {
				SendMessage(pyList, CB_ADDSTRING, 0, (LPARAM)(baseItem + i->first).c_str());
			}
		}

		// Populate ISE list
		if ( !iseInstalls.empty() ) {
			const wstring baseItem = L"Xilinx ISE ";
			for ( vector<pair<wstring, wstring>>::const_iterator i = iseInstalls.begin(); i != iseInstalls.end(); i++ ) {
				SendMessage(iseList, CB_ADDSTRING, 0, (LPARAM)(baseItem + i->first).c_str());
			}
		}
		return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
}

// -------------------------------------------------------------------------------------------------
// Application entry point.
//
int WINAPI WinMain(
	HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	DWORD retVal;
	CoInitialize(NULL);
	try {
		INITCOMMONCONTROLSEX icc = {0,};
		icc.dwSize = sizeof(icc);
		icc.dwICC = ICC_WIN95_CLASSES | ICC_LINK_CLASS;
		InitCommonControlsEx(&icc);

		do {
			Installer installer;
			retVal = DialogBoxParam(
				hInstance, MAKEINTRESOURCE(IDD_MAINDIALOG), NULL,
				&dialogCallback, (LPARAM)&installer
			);
			if ( retVal ==IDOK ) {
				installer.makeLink();
				break;
			}
		} while ( retVal == IDC_REFRESH );

		(void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;
		retVal = 0;
	}
	catch ( const exception &ex ) {
		MessageBoxA(NULL, ex.what(), "Installation failed!", MB_OK|MB_ICONERROR);
		retVal = 1;
	}
	CoUninitialize();
	return retVal;
}

/* rc -nologo resource.rc
 * cl -O1 -Os -EHsc -D UNICODE -D _UNICODE -D _WIN32_IE=0x0500 -D WINVER=0x600 -I. -I/c/makestuff/common main.cpp \
 * kernel32.lib user32.lib ole32.lib shell32.lib advapi32.lib comctl32.lib resource.res
 */
#include <string>
//#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <algorithm>
#include <iterator>
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <winreg.h>
#include <wchar.h>
#include <makestuff.h>
#include "resource.h"

using namespace std;

class InstallerException : public exception {
	string m_message;
	void addEmail(ostringstream &s) {
		s
			<< ".\n\nPlease report this error, including detailed information about"
			<< "\nyour Windows installation to prophet3636" << "@" << "gmail.com.";
	}
public:
	InstallerException(const string &message, DWORD retCode, int line) {
		ostringstream s;
		s
			<< message
			<< " (0x" << hex << uppercase << setw(8) << setfill('0') << retCode << ") at line "
			<< dec << line;
		addEmail(s);
		m_message = s.str();
	}
	InstallerException(const string &message, int line) {
		ostringstream s;
		s << message << " at line " << line;
		addEmail(s);
		m_message = s.str();
	}
	~InstallerException() throw() { }
	const char *what(void) const throw() { return m_message.c_str(); }
};

class RegistryKey {
	HKEY m_key;
	typedef LONG (WINAPI *FuncPtr)(HKEY, DWORD, LPWSTR, PDWORD, const void*, const void*, const void*, const void*);
public:
	enum EnumType { EnumKeys, EnumValues };
	RegistryKey(void) : m_key(NULL) { }
	bool open(HKEY root, const wstring &path) {
		if ( m_key ) {
			RegCloseKey(m_key);
		}
		DWORD status = RegOpenKeyEx(
			root,
			path.c_str(),
			0,
			KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
			&m_key
		);
		if ( status == ERROR_FILE_NOT_FOUND ) {
			m_key = NULL;
			return false;
		} else if ( status == ERROR_SUCCESS ) {
			return true;
		} else {
			throw InstallerException("RegistryKey::open(): RegOpenKeyEx failed", status, __LINE__);
		}
	}
	~RegistryKey(void) {
		if ( m_key ) {
			RegCloseKey(m_key);
		}
	}
	void enumChildren(EnumType operation, vector<wstring> &children) {
		wchar_t buf[MAX_PATH];
		DWORD len = MAX_PATH;
		DWORD index = 0;
		DWORD status;
		FuncPtr func;
		if ( operation == EnumKeys ) {
			func = (FuncPtr)&RegEnumKeyEx;
		} else if ( operation == EnumValues ) {
			func = (FuncPtr)&RegEnumValue;
		} else {
			throw InstallerException("RegistryKey::enumChildren(): Illegal operation", __LINE__);
		}
		status = func(
			m_key,
			index++,
			buf, &len,
			0, NULL, NULL, NULL
		);
		while ( status == ERROR_SUCCESS ) {
			if ( *buf != L'\0' ) {
				children.push_back(buf);
			}
			len = MAX_PATH;
			status = func(
				m_key,
				index++,
				buf, &len,
				0, NULL, NULL, NULL
			);
		}
		if ( status != ERROR_NO_MORE_ITEMS ) {
			throw InstallerException("RegistryKey::enumChildren(): Registry enumeration failed", status, __LINE__);
		}
	}
	wstring getValueAsString(const wstring &name) {
		wchar_t buf[MAX_PATH];
		DWORD len = MAX_PATH;
		DWORD status = RegQueryValueEx(
			m_key,
			name.c_str(),
			NULL,
			NULL,
			(LPBYTE)buf,
			&len
		);
		if ( status != ERROR_SUCCESS ) {
			throw InstallerException("RegistryKey::getValueAsString(): RegQueryValueEx failed", status, __LINE__);
		}
		return buf;
	}
};			

class ShellLink {
	IShellLink *m_psl;
public:
	ShellLink(void) : m_psl(NULL) {
		HRESULT hres = CoCreateInstance(
			CLSID_ShellLink, NULL,
			CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&m_psl
		);
		if ( !SUCCEEDED(hres) ) {
			throw InstallerException("ShellLink::ShellLink(): CoCreateInstance failed", __LINE__);
		}
	}
	~ShellLink(void) {
		if ( m_psl ) {
			m_psl->Release();
		}
	}
	void createShortcut(
		const wchar_t *target, const wchar_t *args, const wchar_t *desc, const wchar_t *path)
	{
		IPersistFile *ppf = NULL;
		m_psl->SetPath(target);
		m_psl->SetArguments(args);
		m_psl->SetDescription(desc);
		HRESULT hres = m_psl->QueryInterface(IID_IPersistFile, (void**)&ppf); 
		if ( !SUCCEEDED(hres) ) {
			throw InstallerException("ShellLink::createShortcut(): can't get IPersistFile interface", __LINE__);;
		}
		hres = ppf->Save(path, TRUE);
		ppf->Release();
		if ( !SUCCEEDED(hres) ) {
			throw InstallerException("ShellLink::createShortcut(): IPersistFile failed", __LINE__);
		}
	}
};

wstring getDesktopPath(void) {
	wchar_t path[MAX_PATH];
	if ( S_OK != SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, path) ) {
		throw InstallerException("getDesktopPath(): Unable to determine the location of your desktop directory", __LINE__);
	}
	return path;
}

void createConsoleConfig(const wchar_t *consoleName) {
	struct Value {
		const wchar_t *name;
		DWORD type;
		const BYTE *value;
		DWORD length;
	};
	const wchar_t *const faceNameStr = L"Lucida Console";
	const DWORD faceNameLen = (DWORD)wcslen(faceNameStr);
	const DWORD cursorSize = 25;
	const DWORD fontFamily = 54;
	const DWORD fontSize = 0xE0000;
	const DWORD fontWeight = 400;
	const DWORD historyBufferSize = 50;
	const DWORD historyNoDup = 0;
	const DWORD insertMode = 1;
	const DWORD numberOfHistoryBuffers = 4;
	const DWORD quickEdit = 1;
	const DWORD screenBufferSize = 0x12C0082;
	const DWORD screenColors = 0x1F;
	const DWORD windowSize = 0x280082;
	const struct Value values[] = {
		{L"CursorSize",             REG_DWORD, (const BYTE *)&cursorSize,             4},
		{L"FaceName",               REG_SZ,    (const BYTE *)faceNameStr, 2*faceNameLen},
		{L"FontFamily",             REG_DWORD, (const BYTE *)&fontFamily,             4},
		{L"FontSize",               REG_DWORD, (const BYTE *)&fontSize,               4},
		{L"FontWeight",             REG_DWORD, (const BYTE *)&fontWeight,             4},
		{L"HistoryBufferSize",      REG_DWORD, (const BYTE *)&historyBufferSize,      4},
		{L"HistoryNoDup",           REG_DWORD, (const BYTE *)&historyNoDup,           4},
		{L"InsertMode",             REG_DWORD, (const BYTE *)&insertMode,             4},
		{L"NumberOfHistoryBuffers", REG_DWORD, (const BYTE *)&numberOfHistoryBuffers, 4},
		{L"QuickEdit",              REG_DWORD, (const BYTE *)&quickEdit,              4},
		{L"ScreenBufferSize",       REG_DWORD, (const BYTE *)&screenBufferSize,       4},
		{L"ScreenColors",           REG_DWORD, (const BYTE *)&screenColors,           4},
		{L"WindowSize",             REG_DWORD, (const BYTE *)&windowSize,             4}
	};
	int i;
	const struct Value *p;
	HKEY hKey;
	DWORD dwDisposition;
	LONG status;
	wstring keyName(L"Console\\");
	keyName += consoleName;

	// First create container key
	status = RegCreateKeyEx(
		HKEY_CURRENT_USER,
		keyName.c_str(),
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,
		NULL,
		&hKey,
		&dwDisposition
	);
	if ( status ) {
		throw InstallerException("createConsoleConfig(): RegCreateKeyEx failed", __LINE__);
	}
	i = sizeof(values)/sizeof(struct Value);
	p = values;
	while ( i-- ) {
		status = RegSetValueEx(
			hKey,
			p->name,
			0,
			p->type,
			p->value,
			p->length
		);
		if ( status ) {
			throw InstallerException("createConsoleConfig(): RegSetValueEx failed", __LINE__);
		}
		p++;
	}
}

bool is64(void) {
	wchar_t buf[100];
	return
		GetEnvironmentVariable(L"PROCESSOR_ARCHITEW6432", buf, 100) == 5 &&
		buf[0] == L'A' &&
		buf[1] == L'M' &&
		buf[2] == L'D' &&
		buf[3] == L'6' &&
		buf[4] == L'4' &&
		buf[5] == L'\0';
}

static const wchar_t *const PY_ROOT = L"Software\\Python\\PythonCore";
static const wchar_t *const VS_ROOT = L"Software\\Microsoft\\VisualStudio\\SxS\\VS7";

class Installer {
	map<wstring, wstring> m_pyInstalls;
	map<wstring, wstring> m_vsInstalls;
	void iterPython(HKEY root) {
		RegistryKey key;
		if ( key.open(root, PY_ROOT ) ) {
			RegistryKey installPath;
			vector<wstring> children;
			key.enumChildren(RegistryKey::EnumKeys, children);
			for ( vector<wstring>::const_iterator i = children.begin(); i != children.end(); i++ ) {
				const wstring &version = *i;
				wstring installPathName = PY_ROOT;
				installPathName += L'\\';
				installPathName += version;
				installPathName += L"\\InstallPath";
				if ( installPath.open(root, installPathName ) ) {
					m_pyInstalls.insert(pair<wstring, wstring>(version, installPath.getValueAsString(L"")));
				}
			}
		}
	}
public:
	Installer(void) {
		RegistryKey key;
		if ( key.open(HKEY_LOCAL_MACHINE, VS_ROOT) ) {
			vector<wstring> children;
			key.enumChildren(RegistryKey::EnumValues, children);
			for ( vector<wstring>::const_iterator i = children.begin(); i != children.end(); i++ ) {
				const wstring &version = *i;
				m_vsInstalls.insert(pair<wstring, wstring>(version, key.getValueAsString(version)));
			}
		}
		iterPython(HKEY_CURRENT_USER);
		iterPython(HKEY_LOCAL_MACHINE);
	}
	void makeLink(void) {
		if ( !m_vsInstalls.empty() ) {
			map<wstring, wstring>::const_iterator vsPair = m_vsInstalls.begin();
			ShellLink shellLink;
			wstring name = L"\\x86-vs";
			name += vsPair->first;
			wstring target = L"/c \"";
			target += vsPair->second;
			if ( m_pyInstalls.empty() ) {
				target += L"VC\\vcvarsall.bat\" x86 && set MACHINE=x86 && C:\\makestuff\\msys\\bin\\bash.exe --login";
			} else {
				map<wstring, wstring>::const_iterator pyPair = m_pyInstalls.begin();
				target += L"VC\\vcvarsall.bat\" x86 && set PATH=";
				target += pyPair->second;
				target += L";%PATH% && set MACHINE=x86 && C:\\makestuff\\msys\\bin\\bash.exe --login";
			}
			const wstring path = getDesktopPath() + name + L".lnk";
			shellLink.createShortcut(
				L"%comspec%",
				target.c_str(),
				L"Build x86 code",
				path.c_str()
			);
			createConsoleConfig(name.c_str());
		}
	}
	/*void dump(void) {
		wcout << L"Visual Studio installations:\n";
		for ( map<wstring, wstring>::const_iterator it = m_vsInstalls.begin(); it != m_vsInstalls.end(); it++ ) {
			wcout << L"  " << it->first << L": " << it->second << endl;
		}
		wcout << L"Python global installations:" << endl;
		for ( map<wstring, wstring>::const_iterator it = m_pyInstalls.begin(); it != m_pyInstalls.end(); it++ ) {
			wcout << L"  " << it->first << L": " << it->second << endl;
		}
	}*/
};
	
INT_PTR CALLBACK dialogCallback(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if ( uMsg == WM_COMMAND ) {
		if ( HIWORD(wParam) == BN_CLICKED ) {
			switch ( LOWORD(wParam) ) {
			case IDCANCEL:
			case IDOK:
				EndDialog(hwndDlg, (INT_PTR)LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
		} else if ( HIWORD(wParam) == CBN_SELCHANGE ) {
			const DWORD selection = SendMessage((HWND)lParam, CB_GETCURSEL, NULL, NULL);
			const DWORD dropdown = LOWORD(wParam);
			const HWND info = GetDlgItem(hwndDlg, dropdown+1);
			ostringstream os;os
				<< "Dropdown ID " << dropdown
				<< ": selected index " << selection;
			MessageBoxA(hwndDlg, os.str().c_str(), "Selection changed", MB_OK|MB_ICONINFORMATION);
			if ( selection == 1 ) {
				SendMessage(info, WM_SETTEXT, 0, (LPARAM)L"Goodbye <a href=\"http://makestuff.eu\">world</a>, how are <a href=\"http://sun.com\">you</a> today?");
				//ShowWindow(GetDlgItem(hwndDlg, IDC_COMPILERINFO), SW_HIDE);
			} else {
				SendMessage(info, WM_SETTEXT, 0, (LPARAM)L"Hello <a href=\"http://makestuff.eu\">world</a>, how are <a href=\"http://microsoft.com\">you</a> today?");
				//ShowWindow(GetDlgItem(hwndDlg, IDC_COMPILERINFO), SW_SHOW);
			}
		}
	} else if ( uMsg == WM_NOTIFY ) {
		const NMLINK *link = (const NMLINK *)lParam;
		switch ( link->hdr.code ) {
			case NM_CLICK:
			case NM_RETURN: {
				const LITEM *item = &link->item;
				wostringstream os;
				os << L"Clicked on link[" << item->iLink << L"]: " << item->szUrl;
				MessageBox(hwndDlg, os.str().c_str(), L"URL click", MB_OK|MB_ICONINFORMATION);
				ShellExecute(NULL, L"open", item->szUrl, NULL, NULL, SW_SHOW);
				break;
			}
		}
	} else if ( uMsg == WM_INITDIALOG ) {
		// Set C/C++ Compilers info text
		HWND child = GetDlgItem(hwndDlg, IDC_COMPILERINFO);
		SendMessage(child, WM_SETTEXT, 0, (LPARAM)L"Hello <a href=\"http://makestuff.eu\">world</a>, how are <a href=\"http://microsoft.com\">you</a> today?");

		// Populate C/C++ Compilers list
		child = GetDlgItem(hwndDlg, IDC_COMPILERLIST);
		SendMessage(child, CB_ADDSTRING, 0, (LPARAM)L"Visual Studio 11.0 x86");
		SendMessage(child, CB_ADDSTRING, 0, (LPARAM)L"Visual Studio 11.0 x64");
		SendMessage(child, CB_ADDSTRING, 0, (LPARAM)L"Visual Studio 12.0 x86");
		SendMessage(child, CB_ADDSTRING, 0, (LPARAM)L"Visual Studio 12.0 x64");
		SendMessage(child, CB_ADDSTRING, 0, (LPARAM)L"Windows SDK 7.1 x86");
		SendMessage(child, CB_ADDSTRING, 0, (LPARAM)L"Windows SDK 7.1 x64");

		// Populate Python Interpreters list
		child = GetDlgItem(hwndDlg, IDC_PYTHONLIST);
		SendMessage(child, CB_ADDSTRING, 0, (LPARAM)L"C:\\Python27");
		SendMessage(child, CB_ADDSTRING, 0, (LPARAM)L"C:\\Python32");

		return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
}

// Our application entry point.
int WINAPI WinMain(
	HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	try {
		int x;
		INITCOMMONCONTROLSEX icc = {0,};
		icc.dwSize = sizeof(icc);
		icc.dwICC = ICC_WIN95_CLASSES | ICC_LINK_CLASS;
		InitCommonControlsEx(&icc);
		
		DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAINDIALOG), NULL, &dialogCallback);
		
		//MessageBoxA(NULL, "Foo bar", "Quitting...", MB_OK|MB_ICONINFORMATION);
		
		(void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;
		return 0;
	}
	catch ( const exception &ex ) {
		MessageBoxA(NULL, ex.what(), "Installation failed!", MB_OK|MB_ICONERROR);
		return 1;
	}
}

/*int main(void) {
	int retVal;
	CoInitialize(NULL);
	try {
		Installer installer;
		installer.dump();
		installer.makeLink();
		wcout << (is64() ? L"x64" : L"x86") << endl;

		retVal = 0;
	}
	catch ( const exception &ex ) {
		cout << ex.what() << endl;
		retVal = 1;
	}
	CoUninitialize();
	return retVal;
}*/

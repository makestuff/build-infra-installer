/* rc -nologo resource.rc; \
 * cl -O1 -Os -EHsc -D UNICODE -D _UNICODE -D _WIN32_IE=0x0500 -D WINVER=0x600 -I. -I/c/makestuff/common main.cpp \
 * kernel32.lib user32.lib ole32.lib shell32.lib advapi32.lib comctl32.lib resource.res
 */
#include <string>
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

// -------------------------------------------------------------------------------------------------
// Class & function declarations.
//
class InstallerException : public exception {
	string m_message;
	static void addEmail(ostringstream &s);
public:
	InstallerException(const string &message, DWORD retCode, int line);
	InstallerException(const string &message, int line);
	~InstallerException() throw() { }
	const char *what(void) const throw() { return m_message.c_str(); }
};
class RegistryKey {
	HKEY m_key;
	typedef LONG (WINAPI *FuncPtr)(HKEY, DWORD, LPWSTR, PDWORD, const void*, const void*, const void*, const void*);
public:
	enum EnumType { EnumKeys, EnumValues };
	RegistryKey(void) : m_key(NULL) { }
	~RegistryKey(void);
	bool open(HKEY root, const wstring &path);
	void enumChildren(EnumType operation, vector<wstring> &children) const;
	wstring getValueAsString(const wstring &name) const;
};			
class ShellLink {
	IShellLink *m_psl;
public:
	ShellLink(void);
	~ShellLink(void);
	void createShortcut(
		const wchar_t *target, const wchar_t *args, const wchar_t *desc, const wchar_t *path
	) const;
};
struct VSInstall {
	const wstring id;
	const wstring name;
	const wstring path;
	VSInstall(const wstring &ci, const wstring &cn, const wstring &cp) :
		id(ci), name(cn), path(cp) { }
	VSInstall(const VSInstall &other) :
		id(other.id), name(other.name), path(other.path) { }
private:
	VSInstall &operator=(const VSInstall &other) { }
};
class Installer {
	vector<pair<wstring, wstring>> m_pyInstalls;
	vector<VSInstall> m_vsInstalls;
	int m_vsSelect;
	int m_pySelect;
	bool m_hdlUseable;
	wstring m_linkName;
	static void iterPython(HKEY root, map<wstring, wstring> &pyInstalls);
	static int getMajorVersion(const wchar_t *str);
public:
	Installer(void);
	void selectionChanged(DWORD id, DWORD sel);
	void hdlUseable(bool yesno) { m_hdlUseable = yesno; }
	const wchar_t *getLinkName(void) const;
	void makeLink(void) const;
	const vector<VSInstall> &getCompilers(void) const { return m_vsInstalls; }
	const vector<pair<wstring, wstring>> &getPythons(void) const { return m_pyInstalls; }
};
void createConsoleConfig(const wchar_t *consoleName);
wstring getDesktopPath(void);
bool is64(void);
bool isFilePresent(const wstring &fileName);

// -------------------------------------------------------------------------------------------------
// InstallerException operations
//
InstallerException::InstallerException(const string &message, DWORD retCode, int line) {
	ostringstream s;
	s
		<< message
		<< " (0x" << hex << uppercase << setw(8) << setfill('0') << retCode << ") at line "
		<< dec << line;
	addEmail(s);
	m_message = s.str();
}
InstallerException::InstallerException(const string &message, int line) {
	ostringstream s;
	s << message << " at line " << line;
	addEmail(s);
	m_message = s.str();
}

void InstallerException::addEmail(ostringstream &s) {
    s
        << ".\n\nPlease report this error, including detailed information about"
        << "\nyour Windows installation to prophet3636" << "@" << "gmail.com.";
}


// -------------------------------------------------------------------------------------------------
// RegistryKey operations
//
bool RegistryKey::open(HKEY root, const wstring &path) {
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
RegistryKey::~RegistryKey(void) {
	if ( m_key ) {
		RegCloseKey(m_key);
	}
}
void RegistryKey::enumChildren(EnumType operation, vector<wstring> &children) const {
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
wstring RegistryKey::getValueAsString(const wstring &name) const {
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

// -------------------------------------------------------------------------------------------------
// ShellLink operations
//
ShellLink::ShellLink(void) : m_psl(NULL) {
	HRESULT hres = CoCreateInstance(
		CLSID_ShellLink, NULL,
		CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&m_psl
	);
	if ( !SUCCEEDED(hres) ) {
		throw InstallerException("ShellLink::ShellLink(): CoCreateInstance failed", __LINE__);
	}
}
ShellLink::~ShellLink(void) {
	if ( m_psl ) {
		m_psl->Release();
	}
}
void ShellLink::createShortcut(
	const wchar_t *target, const wchar_t *args, const wchar_t *desc, const wchar_t *path) const
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

// -------------------------------------------------------------------------------------------------
// Installer operations
//
static const wchar_t *const PY_ROOT = L"Software\\Python\\PythonCore";
static const wchar_t *const VS_ROOT = L"Software\\Microsoft\\VisualStudio\\SxS\\VS7";
static const wchar_t *const SDK_ROOT = L"Software\\Microsoft\\Microsoft SDKs\\Windows\\v7.1";
void Installer::iterPython(HKEY root, map<wstring, wstring> &pyInstalls) {
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
				pyInstalls.insert(make_pair(version, installPath.getValueAsString(L"")));
			}
		}
	}
}
int Installer::getMajorVersion(const wchar_t *str) {
	int result = 0;
	while ( *str && *str != L'.' ) {
		result *= 10;
		result += *str - L'0';
		str++;
	}
	return result;
}
Installer::Installer(void) : m_vsSelect(-1), m_pySelect(-1), m_hdlUseable(false), m_linkName(L"makestuff-base") {
	RegistryKey key;
	const bool x64 = is64();

	// See if Windows SDK v7.1 is installed
	if ( key.open(HKEY_LOCAL_MACHINE, SDK_ROOT) ) {
		wstring target = L"\"";
		target += key.getValueAsString(L"InstallationFolder");
		m_vsInstalls.push_back(
			VSInstall(
				L"sdk7.1.x86",
				L"Windows SDK v7.1 C/C++ Compiler (x86)",
				target + L"Bin\\SetEnv.cmd\" /x86&&set MACHINE=x86&&"
			)
		);
		if ( x64 ) {
			m_vsInstalls.push_back(
				VSInstall(
					L"sdk7.1.x64",
					L"Windows SDK v7.1 C/C++ Compiler (x64)",
					target + L"Bin\\SetEnv.cmd\" /x64&&set MACHINE=x64&&"
				)
			);
		}
	}

	// See which versions (if any) of Visual Studio are installed
	if ( key.open(HKEY_LOCAL_MACHINE, VS_ROOT) ) {
		const wstring baseName = L"Visual Studio ";
		vector<wstring> children;
		key.enumChildren(RegistryKey::EnumValues, children);
		for ( vector<wstring>::const_iterator i = children.begin(); i != children.end(); i++ ) {
			const wstring &version = *i;
			const int majorVersion = getMajorVersion(i->c_str());
			const wstring thisName = baseName + version;
			wstring thisTarget = L"\"";
			thisTarget += key.getValueAsString(version);
			m_vsInstalls.push_back(
				VSInstall(
					L"vs" + version + L".x86",
					thisName + L" C/C++ Compiler (x86)",
					thisTarget + L"VC\\vcvarsall.bat\" x86&&set MACHINE=x86&&"
				)
			);
			if ( x64 && majorVersion > 10 ) {
				// 64-bit compilers are only available after VS2010, and realistically only on Windows x64.
				m_vsInstalls.push_back(
					VSInstall(
						L"vs" + version + L".x64",
						thisName + L" C/C++ Compiler (x64)",
						thisTarget + L"VC\\vcvarsall.bat\" x86_amd64&&set MACHINE=x64&&"
					)
				);
			}
		}
	}

	map<wstring, wstring> pyInstalls;
	iterPython(HKEY_CURRENT_USER, pyInstalls);
	iterPython(HKEY_LOCAL_MACHINE, pyInstalls);
	for ( map<wstring, wstring>::const_iterator i = pyInstalls.begin(); i != pyInstalls.end(); i++ ) {
		m_pyInstalls.push_back(
			make_pair(
				i->first,
				i->second
			)
		);
	}
}
void Installer::selectionChanged(DWORD id, DWORD sel) {
	switch ( id ) {
	case IDC_COMPILERLIST:
		m_vsSelect = (int)sel;
		break;
	case IDC_PYTHONLIST:
		m_pySelect = (int)sel;
		break;
	default:
		throw InstallerException("Installer::selectionChanged(): unrecognised ID", __LINE__);
	}
	vector<wstring> components;
	if ( m_vsSelect != -1 ) {
		components.push_back(m_vsInstalls[m_vsSelect].id);
	}
	if ( m_pySelect != -1 ) {
		components.push_back(L"py" + m_pyInstalls[m_pySelect].first);
	}
	vector<wstring>::const_iterator it = components.begin();
	m_linkName = *it++;
	while ( it != components.end() ) {
		m_linkName += L'-';
		m_linkName += *it++;
	}
}

const wchar_t *Installer::getLinkName(void) const {
	return m_linkName.c_str();
}	

void Installer::makeLink(void) const {
	const wstring path = getDesktopPath() + L"\\" + m_linkName + L".lnk";
	ShellLink shellLink;
	wstring args;
	args = (m_vsSelect == -1) ? L"/k " : L"/c ";
	if ( m_pySelect != -1 ) {
		const pair<wstring, wstring> &pyChoice = m_pyInstalls[m_pySelect];
		args += L"set PATH=" + pyChoice.second + L";%PATH%&&";
	}
	if ( m_vsSelect != -1 ) {
		// A compiler has been selected
		const VSInstall &vsChoice = m_vsInstalls[m_vsSelect];
		args += vsChoice.path;
	}
	args += L"C:\\makestuff\\msys\\bin\\bash.exe --login";
	
	//MessageBox(NULL, args.c_str(), L"Shortcut creation", MB_OK|MB_ICONINFORMATION);
	
	shellLink.createShortcut(
		L"%comspec%",
		args.c_str(),
		L"Build x86 code",
		path.c_str()
	);
	createConsoleConfig(m_linkName.c_str());
}

// -------------------------------------------------------------------------------------------------
// Create a "prettified" console entry in the registry with the given name.
//
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
	const DWORD screenBufferSize = 0x12C0078;
	const DWORD screenColors = 0x0F;
	const DWORD colorTable00 = 0x00400000;
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
		{L"ColorTable00",           REG_DWORD, (const BYTE *)&colorTable00,           4},
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

// -------------------------------------------------------------------------------------------------
// Get the path to the user's desktop.
//
wstring getDesktopPath(void) {
	wchar_t path[MAX_PATH];
	if ( S_OK != SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, path) ) {
		throw InstallerException("getDesktopPath(): Unable to determine the location of your desktop directory", __LINE__);
	}
	return path;
}

// -------------------------------------------------------------------------------------------------
// Determine whether this Windows installation is x86 or x64.
//
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

// -------------------------------------------------------------------------------------------------
// Determine whether a file or directory is present or not.
//
bool isFilePresent(const wstring &fileName) {
	return GetFileAttributes(fileName.c_str()) != INVALID_FILE_ATTRIBUTES;
}

// -------------------------------------------------------------------------------------------------
// Get the canonical grandparent path of the given filename.
//
wstring grandParent(const wstring &str) {
	wchar_t path[MAX_PATH];
	GetFullPathName((str + L"\\..\\..").c_str(), MAX_PATH, path, NULL);
	return path;
}
	
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
				const vector<pair<wstring, wstring>> &pyInstalls = installer->getPythons();
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
		const vector<VSInstall> &vsInstalls = installer->getCompilers();
		const vector<pair<wstring, wstring>> &pyInstalls = installer->getPythons();
		const HWND vsList = GetDlgItem(hwndDlg, IDC_COMPILERLIST);
		const HWND vsInfo = GetDlgItem(hwndDlg, IDC_COMPILERINFO);
		const HWND pyList = GetDlgItem(hwndDlg, IDC_PYTHONLIST);
		const HWND pyInfo = GetDlgItem(hwndDlg, IDC_PYTHONINFO);
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

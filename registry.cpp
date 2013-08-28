//#include <iostream>
#include "registry.h"
#include "exception.h"

using namespace std;

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
//bool RegistryKey::getValueAsString(const wstring &name, wstring *value) const {
//	wchar_t path[name.length() + 1];
//	value.clear();
//	wcscpy(path, name.c_str());
//	return getValueAsString(path, value);
//}
wstring RegistryKey::getValueAsString(const wstring &name) const {
	wstring path = name;
	wstring value;
	//wcscpy(path, name.c_str());
	getValueAsString(&path.front(), value);
	return value;
}
bool RegistryKey::getValueAsString(wchar_t *path, wstring &result) const {
	wchar_t *ptr = path;
	bool retVal;
	while ( *ptr && *ptr != L'\\' ) {
		ptr++;
	}
	if ( *ptr ) {
		// Branch node
		RegistryKey key;
		*ptr++ = L'\0';
		if ( !key.open(m_key, path) ) {
			return false;
		}
		return key.getValueAsString(ptr, result);
	} else {
		// Leaf node
		wchar_t buf[MAX_PATH];
		DWORD len = MAX_PATH;
		const DWORD status = RegQueryValueEx(
			m_key,
			path,
			NULL, NULL,
			(LPBYTE)buf,
			&len
		);
		if ( status == ERROR_FILE_NOT_FOUND ) {
			return false;
		} else if ( status == ERROR_SUCCESS ) {
			result += buf;
			return true;
		} else {
			throw InstallerException("RegistryKey::getValueAsString(): RegQueryValueEx failed", status, __LINE__);
		}
	}
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

/*static const wchar_t *const ISE_ROOT = L"Software\\Xilinx\\ISE";
int main(void) {
	RegistryKey key;
	if ( key.open(HKEY_CURRENT_USER, ISE_ROOT) ) {
		wcout
			<< key.getValueAsString(L"14.4\\Project Navigator\\Project Manager\\Preferences\\PlanAheadBinDirUserVal")
			<< endl;
	}
	return 0;
}*/

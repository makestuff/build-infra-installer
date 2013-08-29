/* 
 * Copyright (C) 2013 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <string>
#include <vector>
#include "registry.h"
#include "exception.h"

using namespace std;

// Open a registry key. May be re-used to open another key afterwards. Return false if the
// key is not found, and throw an exception if some other error occurs (e.g permissioning).
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
		throw InstallerException(
			"RegistryKey::open(): RegOpenKeyEx failed",
			status, __FILE__, __LINE__);
	}
}

// Close the key, if it's open.
//
RegistryKey::~RegistryKey(void) {
	if ( m_key ) {
		RegCloseKey(m_key);
	}
}

// Enumerate either the keys (EnumKeys) or the values (EnumValues) under this key.
//
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
		throw InstallerException(
			"RegistryKey::enumChildren(): Illegal operation",
			__FILE__, __LINE__);
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
		throw InstallerException(
			"RegistryKey::enumChildren(): Registry enumeration failed",
			status, __FILE__, __LINE__);
	}
}

// Public getValueAsString(). Similar to Win32's RegGetValue() which is not available on XP x86.
//
wstring RegistryKey::getValueAsString(const wstring &name) const {
	wstring path = name;
	wstring value;
	getValueAsString(&path.front(), value);
	return value;
}

// Private recursive getValueAsString() used by public method above.
//
bool RegistryKey::getValueAsString(wchar_t *path, wstring &result) const {
	wchar_t *ptr = path;
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
			throw InstallerException(
				"RegistryKey::getValueAsString(): RegQueryValueEx failed",
				status, __FILE__, __LINE__);
		}
	}
}

// Non-member function for creating a console appearance record in the registry.
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
		throw InstallerException(
			"createConsoleConfig(): RegCreateKeyEx failed",
			__FILE__, __LINE__);
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
			throw InstallerException(
				"createConsoleConfig(): RegSetValueEx failed",
				__FILE__, __LINE__);
		}
		p++;
	}
}

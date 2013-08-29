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
#ifndef REGISTRY_H
#define REGISTRY_H

#include <string>
#include <vector>
#include <windows.h>
#include <winreg.h>

class RegistryKey {
	HKEY m_key;
	typedef LONG (WINAPI *FuncPtr)(HKEY, DWORD, LPWSTR, PDWORD, const void*, const void*, const void*, const void*);
	bool getValueAsString(wchar_t *path, std::wstring &result) const;
public:
	enum EnumType { EnumKeys, EnumValues };
	RegistryKey(void) : m_key(NULL) { }
	~RegistryKey(void);
	bool open(HKEY root, const std::wstring &path);
	void enumChildren(EnumType operation, std::vector<std::wstring> &children) const;
	bool getValueAsString(const std::wstring &name, std::wstring &value) const;
	std::wstring getValueAsString(const std::wstring &name) const;
};

void createConsoleConfig(const wchar_t *consoleName);

#endif

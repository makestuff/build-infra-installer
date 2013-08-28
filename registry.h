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

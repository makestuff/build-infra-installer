#include <string>
#include <windows.h>

using namespace std;

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

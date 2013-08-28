#include <string>
#include <windows.h>

using namespace std;

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

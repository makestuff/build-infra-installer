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
#include <windows.h>

using namespace std;

// Determine whether a file or directory is present or not.
//
bool isFilePresent(const wstring &fileName) {
	return GetFileAttributes(fileName.c_str()) != INVALID_FILE_ATTRIBUTES;
}

// Get the canonical grandparent path of the given filename.
//
wstring grandParent(const wstring &str) {
	wchar_t path[MAX_PATH];
	GetFullPathName((str + L"\\..\\..").c_str(), MAX_PATH, path, NULL);
	return path;
}

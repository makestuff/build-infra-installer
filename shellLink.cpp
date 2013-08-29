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
#include "shellLink.h"
#include "exception.h"

using namespace std;

// Create an instance of the IShellLink COM object.
//
ShellLink::ShellLink(void) : m_psl(NULL) {
	HRESULT hres = CoCreateInstance(
		CLSID_ShellLink, NULL,
		CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&m_psl
	);
	if ( !SUCCEEDED(hres) ) {
		throw InstallerException(
			"ShellLink::ShellLink(): CoCreateInstance failed",
			__FILE__, __LINE__);
	}
}

// Release the IShellLink COM object.
//
ShellLink::~ShellLink(void) {
	if ( m_psl ) {
		m_psl->Release();
	}
}

// Create the specified shortcut.
//
void ShellLink::createShortcut(
	const wchar_t *target, const wchar_t *args, const wchar_t *desc, const wchar_t *path) const
{
	IPersistFile *ppf = NULL;
	m_psl->SetPath(target);
	m_psl->SetArguments(args);
	m_psl->SetDescription(desc);
	HRESULT hres = m_psl->QueryInterface(IID_IPersistFile, (void**)&ppf); 
	if ( !SUCCEEDED(hres) ) {
		throw InstallerException(
			"ShellLink::createShortcut(): can't get IPersistFile interface",
			__FILE__, __LINE__);
	}
	hres = ppf->Save(path, TRUE);
	ppf->Release();
	if ( !SUCCEEDED(hres) ) {
		throw InstallerException(
			"ShellLink::createShortcut(): IPersistFile failed",
			__FILE__, __LINE__);
	}
}

// Nonmember utility function to get the path to the user's desktop.
//
wstring getDesktopPath(void) {
	wchar_t path[MAX_PATH];
	if ( S_OK != SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, path) ) {
		throw InstallerException(
			"getDesktopPath(): Unable to determine the location of your desktop directory",
			__FILE__, __LINE__);
	}
	return path;
}

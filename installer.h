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
#ifndef INSTALLER_H
#define INSTALLER_H

#include <map>
#include <vector>
#include <string>
#include "vsInstall.h"

struct HKEY__;
typedef struct HKEY__ *HKEY;

class Installer {
	std::vector<std::pair<std::wstring, std::wstring>> m_pyInstalls;
	std::vector<VSInstall> m_vsInstalls;
	std::vector<std::pair<std::wstring, std::wstring>> m_iseInstalls;
	int m_vsSelect;
	int m_pySelect;
	int m_iseSelect;
	bool m_hdlUseable;
	const bool m_x64;
	std::wstring m_linkName;
	static void iterPython(HKEY root, std::map<std::wstring, std::wstring> &pyInstalls);
	static int getMajorVersion(const wchar_t *str);
	static bool det64(void);
public:
	Installer(void);
	void selectionChanged(unsigned long id, unsigned long sel);
	void hdlUseable(bool yesno) { m_hdlUseable = yesno; }
	const wchar_t *getLinkName(void) const;
	void makeLink(void) const;
	const std::vector<VSInstall> &getCompilerList(void) const { return m_vsInstalls; }
	const std::vector<std::pair<std::wstring, std::wstring>> &getPythonList(void) const { return m_pyInstalls; }
	const std::vector<std::pair<std::wstring, std::wstring>> &getXilinxList(void) const { return m_iseInstalls; }
	bool is64(void) const { return m_x64; }
};

#endif

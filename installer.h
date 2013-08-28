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

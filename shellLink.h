#ifndef SHELLLINK_H
#define SHELLLINK_H

#include <string>
#include <shlobj.h>

class ShellLink {
	IShellLink *m_psl;
public:
	ShellLink(void);
	~ShellLink(void);
	void createShortcut(
		const wchar_t *target, const wchar_t *args, const wchar_t *desc, const wchar_t *path
	) const;
};

// Get the path to the user's desktop.
std::wstring getDesktopPath(void);

#endif

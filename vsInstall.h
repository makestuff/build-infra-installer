#ifndef VSINSTALL_H
#define VSINSTALL_H

#include <string>

struct VSInstall {
	const std::wstring id;
	const std::wstring name;
	const std::wstring path;
	VSInstall(const std::wstring &ci, const std::wstring &cn, const std::wstring &cp) :
		id(ci), name(cn), path(cp) { }
	VSInstall(const VSInstall &other) :
		id(other.id), name(other.name), path(other.path) { }
private:
	VSInstall &operator=(const VSInstall &other) { (void)other; }
};

#endif

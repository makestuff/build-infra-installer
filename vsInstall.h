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

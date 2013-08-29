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
#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <exception>
#include <string>
#include <sstream>

class InstallerException : public std::exception {
	std::string m_message;
	static void addEmail(std::ostringstream &s);
public:
	InstallerException(
		const std::string &message, unsigned long retCode,
		const char *file, int line);
	InstallerException(
		const std::string &message, const char *file, int line);
	~InstallerException() throw() { }
	const char *what(void) const throw() {
		return m_message.c_str();
	}
};

#endif

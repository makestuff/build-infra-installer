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
#include <sstream>
#include <iomanip>
#include "exception.h"

using namespace std;

// Construct an exception, including a returncode.
//
InstallerException::InstallerException(
	const string &message, unsigned long retCode, const char *file, int line)
{
	ostringstream os; os
		<< message
		<< " (0x" << hex << uppercase << setw(8) << setfill('0') << retCode << ") at "
		<< file << ":" << dec << line;
	addEmail(os);
	m_message = os.str();
}

// Construct an exception without the returncode.
//
InstallerException::InstallerException(
	const string &message, const char *file, int line)
{
	ostringstream os; os << message << " at " << file << ":" << line;
	addEmail(os);
	m_message = os.str();
}

// Private static function to append an "email me maybe" message.
//
void InstallerException::addEmail(ostringstream &os) {
	os
		<< ".\n\nPlease report this error, including detailed information about"
		<< "\nyour Windows installation to prophet3636" << "@" << "gmail.com.";
}

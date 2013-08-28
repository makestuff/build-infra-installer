#include <string>
#include <sstream>
#include <iomanip>
#include "exception.h"

using namespace std;

InstallerException::InstallerException(const string &message, unsigned long retCode, int line) {
	ostringstream s;
	s
		<< message
		<< " (0x" << hex << uppercase << setw(8) << setfill('0') << retCode << ") at line "
		<< dec << line;
	addEmail(s);
	m_message = s.str();
}
InstallerException::InstallerException(const string &message, int line) {
	ostringstream s;
	s << message << " at line " << line;
	addEmail(s);
	m_message = s.str();
}

void InstallerException::addEmail(ostringstream &s) {
    s
        << ".\n\nPlease report this error, including detailed information about"
        << "\nyour Windows installation to prophet3636" << "@" << "gmail.com.";
}

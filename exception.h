#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <exception>
#include <string>
#include <sstream>

class InstallerException : public std::exception {
	std::string m_message;
	static void addEmail(std::ostringstream &s);
public:
	InstallerException(const std::string &message, unsigned long retCode, int line);
	InstallerException(const std::string &message, int line);
	~InstallerException() throw() { }
	const char *what(void) const throw() { return m_message.c_str(); }
};

#endif

#ifndef UTIL_H
#define UTIL_H

#include <string>

bool is64(void);
bool isFilePresent(const std::wstring &fileName);
std::wstring grandParent(const std::wstring &str);

#endif

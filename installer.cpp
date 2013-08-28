#include "installer.h"
#include "registry.h"
#include "exception.h"
#include "shellLink.h"
#include "util.h"
#include "resource.h"

using namespace std;

static const wchar_t *const PY_ROOT = L"Software\\Python\\PythonCore";
static const wchar_t *const VS_ROOT = L"Software\\Microsoft\\VisualStudio\\SxS\\VS7";
static const wchar_t *const SDK_ROOT = L"Software\\Microsoft\\Microsoft SDKs\\Windows\\v7.1";
static const wchar_t *const ISE_ROOT = L"Software\\Xilinx\\ISE";
void Installer::iterPython(HKEY root, map<wstring, wstring> &pyInstalls) {
	RegistryKey key;
	if ( key.open(root, PY_ROOT ) ) {
		RegistryKey installPath;
		vector<wstring> children;
		key.enumChildren(RegistryKey::EnumKeys, children);
		for ( vector<wstring>::const_iterator i = children.begin(); i != children.end(); i++ ) {
			const wstring &version = *i;
			wstring installPathName = PY_ROOT;
			installPathName += L'\\';
			installPathName += version;
			installPathName += L"\\InstallPath";
			if ( installPath.open(root, installPathName ) ) {
				pyInstalls.insert(make_pair(version, installPath.getValueAsString(L"")));
			}
		}
	}
}
int Installer::getMajorVersion(const wchar_t *str) {
	int result = 0;
	while ( *str && *str != L'.' ) {
		result *= 10;
		result += *str - L'0';
		str++;
	}
	return result;
}
// Determine whether this Windows installation is x86 or x64.
bool Installer::det64(void) {
	wchar_t buf[100];
	return
		GetEnvironmentVariable(L"PROCESSOR_ARCHITEW6432", buf, 100) == 5 &&
		buf[0] == L'A' &&
		buf[1] == L'M' &&
		buf[2] == L'D' &&
		buf[3] == L'6' &&
		buf[4] == L'4' &&
		buf[5] == L'\0';
}

Installer::Installer(void) :
	m_vsSelect(-1), m_pySelect(-1), m_iseSelect(-1), m_x64(det64()),
	m_hdlUseable(false), m_linkName(L"makestuff-base")
{
	RegistryKey key;

	// See if Windows SDK v7.1 is installed
	if ( key.open(HKEY_LOCAL_MACHINE, SDK_ROOT) ) {
		wstring target = L"\"";
		target += key.getValueAsString(L"InstallationFolder");
		m_vsInstalls.push_back(
			VSInstall(
				L"sdk7.1.x86",
				L"Windows SDK v7.1 C/C++ Compiler (x86)",
				target + L"Bin\\SetEnv.cmd\" /x86&&set MACHINE=x86&&"
			)
		);
		if ( m_x64 ) {
			m_vsInstalls.push_back(
				VSInstall(
					L"sdk7.1.x64",
					L"Windows SDK v7.1 C/C++ Compiler (x64)",
					target + L"Bin\\SetEnv.cmd\" /x64&&set MACHINE=x64&&"
				)
			);
		}
	}

	// See which versions (if any) of Visual Studio are installed
	if ( key.open(HKEY_LOCAL_MACHINE, VS_ROOT) ) {
		const wstring baseName = L"Visual Studio ";
		vector<wstring> children;
		key.enumChildren(RegistryKey::EnumValues, children);
		for ( vector<wstring>::const_iterator i = children.begin(); i != children.end(); i++ ) {
			const wstring &version = *i;
			const int majorVersion = getMajorVersion(i->c_str());
			const wstring thisName = baseName + version;
			wstring thisTarget = L"\"";
			thisTarget += key.getValueAsString(version);
			m_vsInstalls.push_back(
				VSInstall(
					L"vs" + version + L".x86",
					thisName + L" C/C++ Compiler (x86)",
					thisTarget + L"VC\\vcvarsall.bat\" x86&&set MACHINE=x86&&"
				)
			);
			if ( m_x64 && majorVersion > 10 ) {
				// 64-bit compilers are only available after VS2010, and realistically only on Windows x64.
				m_vsInstalls.push_back(
					VSInstall(
						L"vs" + version + L".x64",
						thisName + L" C/C++ Compiler (x64)",
						thisTarget + L"VC\\vcvarsall.bat\" x86_amd64&&set MACHINE=x64&&"
					)
				);
			}
		}
	}

	// See which versions (if any) of Python are installed
	map<wstring, wstring> pyInstalls;
	iterPython(HKEY_CURRENT_USER, pyInstalls);
	iterPython(HKEY_LOCAL_MACHINE, pyInstalls);
	for ( map<wstring, wstring>::const_iterator i = pyInstalls.begin(); i != pyInstalls.end(); i++ ) {
		m_pyInstalls.push_back(
			make_pair(
				i->first,
				i->second
			)
		);
	}

	// See which versions (if any) of Xilinx ISE are installed
	if ( key.open(HKEY_CURRENT_USER, ISE_ROOT) ) {
		const wstring baseName = L"Xilinx ISE ";
		vector<wstring> children;
		key.enumChildren(RegistryKey::EnumKeys, children);
		for ( vector<wstring>::const_iterator i = children.begin(); i != children.end(); i++ ) {
			const wstring &version = *i;
			const wstring thisName = baseName + version;
			const wstring thisTarget = grandParent(
				key.getValueAsString(
					version +
					L"\\Project Navigator\\Project Manager\\Preferences\\PlanAheadBinDirUserVal"
				)
			) + (m_x64 ? L"\\ISE\\bin\\nt64" : L"\\ISE\\bin\\nt");
			MessageBox(NULL, thisTarget.c_str(), L"ISE", MB_OK|MB_ICONINFORMATION);
			m_iseInstalls.push_back(
				make_pair(version, thisTarget)
			);
			//	VSInstall(
			//		L"vs" + version + L".x86",
			//		thisName + L" C/C++ Compiler (x86)",
			//		thisTarget + L"VC\\vcvarsall.bat\" x86&&set MACHINE=x86&&"
			//	)
			//);
			//if ( x64 && majorVersion > 10 ) {
			//	// 64-bit compilers are only available after VS2010, and realistically only on Windows x64.
			//	m_vsInstalls.push_back(
			//		VSInstall(
			//			L"vs" + version + L".x64",
			//			thisName + L" C/C++ Compiler (x64)",
			//			thisTarget + L"VC\\vcvarsall.bat\" x86_amd64&&set MACHINE=x64&&"
			//		)
			//	);
			//}
		}
	}
}
void Installer::selectionChanged(DWORD id, DWORD sel) {
	switch ( id ) {
	case IDC_COMPILERLIST:
		m_vsSelect = (int)sel;
		break;
	case IDC_PYTHONLIST:
		m_pySelect = (int)sel;
		break;
	case IDC_XILINXLIST:
		m_iseSelect = (int)sel;
		break;
	default:
		throw InstallerException("Installer::selectionChanged(): unrecognised ID", __LINE__);
	}
	vector<wstring> components;
	if ( m_vsSelect != -1 ) {
		components.push_back(m_vsInstalls[m_vsSelect].id);
	}
	if ( m_pySelect != -1 ) {
		components.push_back(L"py" + m_pyInstalls[m_pySelect].first);
	}
	if ( m_iseSelect != -1 ) {
		components.push_back(L"ise" + m_iseInstalls[m_iseSelect].first);
	}
	vector<wstring>::const_iterator it = components.begin();
	m_linkName = *it++;
	while ( it != components.end() ) {
		m_linkName += L'-';
		m_linkName += *it++;
	}
}

const wchar_t *Installer::getLinkName(void) const {
	return m_linkName.c_str();
}	

void Installer::makeLink(void) const {
	const wstring path = getDesktopPath() + L"\\" + m_linkName + L".lnk";
	ShellLink shellLink;
	wstring args;
	args = m_x64 ? L"/c set HOSTTYPE=x64&&" : L"/c set HOSTTYPE=x86&&";
	if ( m_pySelect != -1 ) {
		const pair<wstring, wstring> &pyChoice = m_pyInstalls[m_pySelect];
		args += L"set PATH=" + pyChoice.second;
		if ( m_iseSelect != -1 ) {
			args += L';';
			args += m_iseInstalls[m_iseSelect].second;
		}
		args += L";%PATH%&&";
	} else if ( m_iseSelect != -1 ) {
		args += L"set PATH=" + m_iseInstalls[m_iseSelect].second + L";%PATH%&&";
	}
	if ( m_vsSelect != -1 ) {
		// A compiler has been selected
		const VSInstall &vsChoice = m_vsInstalls[m_vsSelect];
		args += vsChoice.path;
	}
	args += L"C:\\makestuff\\msys\\bin\\bash.exe --login";
	
	//MessageBox(NULL, args.c_str(), L"Shortcut creation", MB_OK|MB_ICONINFORMATION);
	
	shellLink.createShortcut(
		L"%comspec%",
		args.c_str(),
		L"Build x86 code",
		path.c_str()
	);
	createConsoleConfig(m_linkName.c_str());
}

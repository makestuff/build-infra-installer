#include "shellLink.h"
#include "exception.h"

using namespace std;

ShellLink::ShellLink(void) : m_psl(NULL) {
	HRESULT hres = CoCreateInstance(
		CLSID_ShellLink, NULL,
		CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&m_psl
	);
	if ( !SUCCEEDED(hres) ) {
		throw InstallerException("ShellLink::ShellLink(): CoCreateInstance failed", __LINE__);
	}
}
ShellLink::~ShellLink(void) {
	if ( m_psl ) {
		m_psl->Release();
	}
}
void ShellLink::createShortcut(
	const wchar_t *target, const wchar_t *args, const wchar_t *desc, const wchar_t *path) const
{
	IPersistFile *ppf = NULL;
	m_psl->SetPath(target);
	m_psl->SetArguments(args);
	m_psl->SetDescription(desc);
	HRESULT hres = m_psl->QueryInterface(IID_IPersistFile, (void**)&ppf); 
	if ( !SUCCEEDED(hres) ) {
		throw InstallerException("ShellLink::createShortcut(): can't get IPersistFile interface", __LINE__);;
	}
	hres = ppf->Save(path, TRUE);
	ppf->Release();
	if ( !SUCCEEDED(hres) ) {
		throw InstallerException("ShellLink::createShortcut(): IPersistFile failed", __LINE__);
	}
}

wstring getDesktopPath(void) {
	wchar_t path[MAX_PATH];
	if ( S_OK != SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, path) ) {
		throw InstallerException("getDesktopPath(): Unable to determine the location of your desktop directory", __LINE__);
	}
	return path;
}

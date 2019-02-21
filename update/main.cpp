#include "UpdateEx.h"
HINSTANCE g_hInstance = NULL;
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	g_hInstance = hInstance;
	int argCount;
	LPWSTR* szArglist = nullptr;
	szArglist = CommandLineToArgvW(lpCmdLine, &argCount);
	for (size_t i=0;i<argCount;i++)
	{
		::OutputDebugString(szArglist[i]);
		::OutputDebugString(L"\r\n");
	}
	if (argCount < 2)
		return 0;
	CUpdateEx* wkeWindow = CUpdateEx::GetInstance();
	std::string strDownloadUrl = wkeWindow->Unicode_To_Ansi(szArglist[0]);
	if (strDownloadUrl.empty())
	{
		::MessageBox(::GetDesktopWindow(), L"在线升级出现问题即将为您跳转官网下载", NULL, NULL);
		::ShellExecuteA(NULL, "open", "https://www.baiducdp.com/", NULL, NULL, SW_SHOWNORMAL);
		wkeFinalize();
		return 0;
	}
	wkeWindow->SetDownloadUrl(strDownloadUrl);
	wkeWindow->SetSavePath(wkeWindow->Unicode_To_Ansi(szArglist[1]));
	wkeWindow->RunApp();
	wkeFinalize();
	return 0;
}
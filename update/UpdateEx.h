#ifndef __CUPDATE_EX__
#define __CUPDATE_EX__
#include <windows.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "../wke.h"
typedef struct {
	wkeWebView window;
	std::wstring url;
} Application;
extern HINSTANCE g_hInstance;
typedef std::function<int(void*, double, double, double, double)> PROGRESS_CALLBACK;
#define MBDLL_NAME L"node.dll"
#define MAIN_APP_PROCESS_NAME L"BaiduCdp.exe"
#define HOST_NAME "http://baiducdp.com"
#define UPDATE_PROGRESS_VALUE WM_USER + 0x205
#define UI_QUIT_MSG WM_USER +0x206
class CdownLoad;
class CUpdateEx
{
public:
	~CUpdateEx();
	CUpdateEx(const CUpdateEx&) = delete;
	CUpdateEx& operator =(const CUpdateEx&) = delete;
	static CUpdateEx* GetInstance();
	static std::shared_ptr<CUpdateEx> _Instaance;
	void RunApp();
	void SetDownloadUrl(const std::string& strUrl);
	void SetSavePath(const std::string& strPath);
	std::string GetTextMid(const std::string strSource, const std::string strLeft, const std::string strRight);
	std::string Unicode_To_Ansi(const wchar_t* szbuff);
	std::string Gbk_To_Utf8(const char* szBuff);
private:
	//设置主窗口的新回调函数
	static LRESULT CALLBACK MainProc(
		_In_ HWND   hwnd,
		_In_ UINT   uMsg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam
	);
	CUpdateEx();
	static	WNDPROC m_oldProc;
	std::wstring getResourcesPath(const std::wstring name);
	void readJsFile(const wchar_t* path, std::vector<char>* buffer);
	std::string CarkUrl(const char* url);
	std::wstring utf8ToUtf16(const std::string& utf8String);
	std::shared_ptr<CdownLoad> m_downloadPtr;
	HWND m_hwnd;
	std::string m_downloadUrl;
	std::string m_UnZipPath;
	Application app;
public:
	bool InintMiniBlink();
};
class CdownLoad 
{
public:
	explicit CdownLoad(const std::string& strUrl,const std::string& strPath);
	void start(HWND progress_callbackex);
	~CdownLoad();
private:
	std::string m_strUrl;
	std::string m_strPath;
	bool detectionProcessIsExist(const TCHAR* szProcessName);	//检测进程是否存在
	static bool m_isDownloadSucceed;
	static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);
	static int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
};
#endif

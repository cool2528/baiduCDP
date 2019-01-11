#ifndef __GLOBALHEADER__
#define __GLOBALHEADER__
#include "wke.h"
#include <windows.h>
#include <string>
#include <vector>
typedef struct {
	wkeWebView window;
	std::wstring url;
} Application;
#define MBDLL_NAME L"node.dll"
#define APP_NAME L"BaiduCdp"
#define CLOSE_MSG "close"
#define MAX_MSG "max"
#define MIN_MSG "min"
#define HOST_NAME "http://baiducdp.com"
extern Application app;
#endif

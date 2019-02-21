#ifndef __WKEWINDOW__
#define __WKEWINDOW__
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include "GlobalHeader.h"
#include "PanParse.h"
#include "shadow_window.h"
#include "glog/logging.h"
// #define GOOGLE_GLOG_DLL_DECL
// #define GLOG_NO_ABBREVIATED_SEVERITIES
#ifndef _DEBUG
#pragma comment(lib,"glog.lib")
#else
#pragma  comment(lib,"glogd.lib")
#endif
typedef websocketpp::client<websocketpp::config::asio_client> client;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
// pull out the type of messages sent by our config
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
typedef client::connection_ptr connection_ptr;
typedef struct downFileListInfo
{
	std::string connections; //当前下载链接服务器数量
	std::string DownloadSpeed; // 下载速度
	size_t Downloadpercentage;//下载进度百分比
	std::string strFileName;
	std::string strFileGid;
	UINT nErrCode;
	std::string strErrMessage;
	std::string strStatus;	//当前文件的下载状态
	std::string strPath;	//当前文件存放路径
}DOWNFILELISTINFO;
typedef std::vector<DOWNFILELISTINFO> ActiceDownloadList;
/*
下载任务当前的状态
active目前正在下载
waiting用于队列中的下载; 下载未启动
paused暂停下载。
error对于因错误而停止的下载。
complete停止和完成下载。
removed用户删除的下载
*/
#define ARIA2_STATUS_ACTIVE "active"
#define ARIA2_STATUS_WAITING "waiting"
#define ARIA2_STATUS_PAUSED "paused"
#define ARIA2_STATUS_ERROR "error"
#define ARIA2_STATUS_COMPLETE "complete"
#define ARIA2_STATUS_REMOVED "removed"
typedef struct downloadStatus
{
	std::string strFileGid;	//下载任务唯一的标识符
	std::string strFileStatus;
}DOWNLOADSTATUS;
/*下载失败重试次数*/
typedef struct retryCount
{
	std::string strFileName;
	UINT nCount;
}RETRYCOUNT;
/*
自定义消息
*/
#define ARIA2_ADDURL_MSG WM_USER + 0x101
#define ARIA2_DOWNLOAD_START_MSG WM_USER + 0x102
#define ARIA2_DOWNLOAD_COMPLATE_MSG WM_USER + 0x103
#define ARIA2_DOWNLOAD_PAUSE_MSG WM_USER + 0x104
#define ARIA2_DOWNLOAD_STOP_MSG WM_USER + 0x105
#define ARIA2_DOWNLOAD_ERROR_MSG WM_USER + 0x106
#define ARIA2_UPDATE_TELLACTIVE_LIST_MSG WM_USER +0x107
#define ARIA2_UPDATE_TELLERROR_LIST_MSG WM_USER +0x108
#define ARIA2_PURGEDOWNLOAD_MSG WM_USER +0x109
#define ARIA2_RETRYADDURL_MSG WM_USER +0x110
#define UI_DOWNLOAD_SHARE_UPDATE_LIST WM_USER + 0x111
#define UI_UPDATE_FOLODER_LIST_MSG WM_USER + 0x112
#define UI_UPDATE_OFF_LINE_LIST_MSG WM_USER + 0x113
#define UI_UPDATE_USER_FILE_DATA_MSG WM_USER +0x114
#define UI_QUIT_MSG WM_USER +0x115
/*
自定义消息结束
*/
/*Aria2下载状态*/
#define ARIA2_DOWNLOAD_START "aria2.onDownloadStart"
#define ARIA2_DOWNLOAD_COMPLATE "aria2.onDownloadComplete"
#define ARIA2_DOWNLOAD_PAUSE "aria2.onDownloadPause"
#define ARIA2_DOWNLOAD_STOP "aria2.onDownloadStop"
#define ARIA2_DOWNLOAD_ERROR "aria2.onDownloadError"
/*Aria2下载状态end*/
/*发送查询的固定数据*/
#define ARIA2_TELLACTICE_SENDDATA "{\"method\":\"aria2.tellActive\",\"params\":[\"token:CDP\",[\"gid\",\"status\",\"files\",\"totalLength\",\"completedLength\",\"downloadSpeed\",\"connections\",\"errorCode\",\"errorMessage\"]],\"id\":1,\"jsonrpc\":\"2.0\"}"
#define ARIA2_TELLSTATUS_SENDDATA "{\"method\":\"aria2.tellStatus\",\"params\":[\"token:CDP\",\"%1%\",[\"gid\",\"status\",\"files\",\"totalLength\",\"completedLength\",\"downloadSpeed\",\"connections\",\"errorCode\",\"errorMessage\"]],\"id\":1,\"jsonrpc\":\"2.0\"}"
#define ARIA2_GETGLOBAL_STATUS "{\"method\":\"aria2.getGlobalStat\",\"params\":[\"token:CDP\"],\"id\":1,\"jsonrpc\":\"2.0\"}"
#define ARIA2_TELLSTOPPED "{\"method\":\"aria2.tellStopped\",\"params\":[\"token:CDP\",0,%1%,[\"gid\",\"status\",\"totalLength\",\"completedLength\",\"downloadSpeed\",\"connections\",\"errorCode\",\"errorMessage\",\"files\"]],\"id\":1,\"jsonrpc\":\"2.0\"}"
#define ARIA2_TELLWAITING "{\"method\":\"aria2.tellWaiting\",\"params\":[\"token:CDP\",0,%1%,[\"gid\",\"status\",\"totalLength\",\"completedLength\",\"downloadSpeed\",\"connections\",\"errorCode\",\"errorMessage\",\"files\"]],\"id\":1,\"jsonrpc\":\"2.0\"}"
#define ARIA2_PURGEDOWNLOAD_RESULT "{\"method\":\"aria2.removeDownloadResult\",\"params\":[\"token:CDP\",\"%1%\"],\"id\":1,\"jsonrpc\":\"2.0\"}"
#define ARIA2_UNPAUSE "{\"method\":\"aria2.unpause\",\"params\":[\"token:CDP\",\"%1%\"],\"id\":1,\"jsonrpc\":\"2.0\"}"
#define ARIA2_PAUSE "{\"method\":\"aria2.pause\",\"params\":[\"token:CDP\",\"%1%\"],\"id\":1,\"jsonrpc\":\"2.0\"}"
#define ARIA2_REMOVE "{\"method\":\"aria2.remove\",\"params\":[\"token:CDP\",\"%1%\"],\"id\":1,\"jsonrpc\":\"2.0\"}"
#define ARIA2_FORCEPAUSEALL "{\"method\":\"aria2.forcePauseAll\",\"params\":[\"token:CDP\"],\"id\":1,\"jsonrpc\":\"2.0\"}"
#define ARIA2_UNAUSEALL "{\"method\":\"aria2.unpauseAll\",\"params\":[\"token:CDP\"],\"id\":1,\"jsonrpc\":\"2.0\"}"
//ARIA2http请求协议
#define ARIA2_HTTP_REQUESTURL "http://127.0.0.1:6800/jsonrpc"
//用定时器更新发送查询数据
#define UPDTAE_UI_TIMEID 508
//API请求主域名
#define API_DOMAIN_NAME "https://api.baiducdp.com"
typedef struct DownloadSpeedinfo
{
	std::string strUnit;	//单位
	float dwSize;
}DOWNLOADSPEEDINFO;
enum ShadowStatus
{
	SS_ENABLED = 1,
	SS_VISABLE = 1 << 1,	
	SS_PARENTVISIBLE = 1 << 2,	
	SS_DISABLEDBYAERO = 1 << 3
};
class CWkeWindow
{
public:
	~CWkeWindow();
	static CWkeWindow* GetInstance();
private:
	explicit CWkeWindow();
	CWkeWindow(const CWkeWindow&);
	CWkeWindow& operator =(const CWkeWindow&);
	static std::shared_ptr<CWkeWindow> Instance;
private:
	
	/*
	判断是否触发了 百度登录成功回调
	*/
	bool isLogin;
	/*
	保存百度登录后的有效Cookie
	*/
	std::string strCookies;
	/*百度网盘解析相关*/
	CBaiduParse m_BaiduPare;
	/*
	阴影窗口
	*/
	ShadowWnd* m_ShadowWnd;
	/*
	Wss请求相关
	*/
	client m_WssClient;
	/*主窗口的窗口句柄*/
	HWND m_hwnd;
	//设置主窗口的新回调函数
	static LRESULT CALLBACK MainProc(
		_In_ HWND   hwnd,
		_In_ UINT   uMsg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam
		);
 static	WNDPROC m_oldProc;
 //分配内存数据用来投递这块内存需要接收者自行释放
 void* AlloclocalHeap(const std::string& strBuffer);
 //aria2服务端发来的数据
 void ParseAria2JsonInfo(const std::string& strJSon);
 //aria2当前活动的下载数量
 size_t numActive;
 //aria2 当前停止的下载数量
 size_t numStopped;
 //aria2 当前等待下载的数量
 size_t numWaiting;
 //下载文件保存的路径
 std::string m_downloadSavePath;
 //获取文件大小类型
 inline DOWNLOADSPEEDINFO GetFileSizeType(double dSize);
 //取百分比
 inline double Getpercentage(double completedLength, double totalLength);
 //下载失败重试次数的数组
 std::vector<RETRYCOUNT> m_RetryArray;
 //保存分享链接下载文件的信息
 std::vector<REQUESTINFO> m_ShareFileArray;
 //添加分享下载信息到数组
 void AddShareFileItem(REQUESTINFO item);
 //获取需要的元素信息
void GetShareFileItem(const std::string& strFileName, REQUESTINFO& Result);
 //判断重试的次数是否已经用完
 bool IsRetryCountFinish(const std::string& strFileName);
 //判断需要重试的任务是否存在不存在就创建
 bool IsRetryExist(const std::string& strFileName);
 //修改重试的次数
 void addRetryCount(const std::string& strFileName);
 //根据唯一标识符从UI界面获取文件的信息
 DOWNFILELISTINFO GetFileCompletedInfo(const std::string& strGid);
 //解析下载完成后的json获取文件名
 DOWNFILELISTINFO GetTellStatusFileName(const std::string& strJSon);
 private:
#if 1
	/*wss请求相关回调开始*/
	//socket init处理程序
	void on_socket_init(websocketpp::connection_hdl);
	//接收服务端发来的消息
	void on_message(websocketpp::connection_hdl hdl, message_ptr);
	//客户端与服务端建立连接时的回调
	void on_open(websocketpp::connection_hdl hdl);
	//当收到关闭时的回调
	void on_close(websocketpp::connection_hdl);
	//收到失败消息时的回调
	void on_fail(websocketpp::connection_hdl hdl);
	//与服务器连接成功后返回连接句柄
	websocketpp::connection_hdl m_hdl;
	//启动与服务端的链接
	void start(std::string uri);
	//连接到aria2的WSS服务端
	void Connect();
	//发送文本数据给服务端
	 void SendText(std::string& strText);
	/*wss请求相关回调结束*/
#endif
	//启动aria2
	 BOOL RunAria2();
	//wss运行需要单独启动一个线程
	std::shared_ptr<std::thread> m_RunThreadPtr;
	//发送数据给Aria2的定时器回调函数
	static void CALLBACK TimeProc(HWND hwnd, UINT message, UINT iTimerID, DWORD dwTime);

public:
	/*
	@启动程序
	*/
	void runApp(Application* app);
	/*
	@ 构造浏览器需要的常用环境设置各种回调函数
	*/
	bool createWebWindow(Application* app);
	/*
	消息循环
	*/
	void runMessageLoop(Application* app);
	/*
	退出程序
	*/
	void quitApplication(Application* app);
	/*
	获取webWindow窗口句柄
	*/
	HWND getHWND();
	void blinkMaximize();
	void blinkMinimize();
	void blinkClose();
	/*
	系统菜单JS回调函数
	*/
	static jsValue SysMenuJsNativeFunction(jsExecState es, void* param);

	/*
	退出当前账号
	*/
	static jsValue LogOut(jsExecState es, void* param);
	/*枚举用户当前账户所有文件夹*/
	static jsValue EnumFolder(jsExecState es, void* param);
	/*获取当前离线下载的列表或者添加离线下载任务*/
	static jsValue OffLineDownload (jsExecState es, void* param);
	/*
	验证是否已经有效的登录了百度网盘
	*/
	static jsValue IsLoginBaidu(jsExecState es, void* param);
	/*
	暂停/恢复/删除下载
	*/
	static jsValue AraiaPauseStartRemove(jsExecState es, void* param);
	/*
	分享文件回调函数
	*/
	static jsValue ShareBaiduFile(jsExecState es, void* param);
	/*重命名百度用户文件*/
	static jsValue BaiduFileRename(jsExecState es, void* param);
	/*
	删除文件或者文件夹
	*/
	static jsValue DeleteBaiduFile(jsExecState es, void* param);
	/*
	点击了关闭的回调函数、返回 true 将销毁窗口，返回 false 什么都不做。
	*/
	static bool OnCloseCallBack(wkeWebView webWindow, void* param);
	/*打开文件所在目录*/
	static jsValue OpenFilePlaceFolder(jsExecState es, void* param);
	/*切换分享链接下载目录*/
	static jsValue switchShareFolder(jsExecState es, void* param);
	/*
	回调：窗口已销毁
	*/
	static void OnWindowDestroy(wkeWebView webWindow, void* param);
	/*
	回调：文档加载成功
	*/
	static void OnDocumentReady(wkeWebView webWindow, void* param);
	/*
	回调 发送网页请求时的回调 可以用来hook
	*/
	static bool onLoadUrlBegin(wkeWebView webView, void* param, const char* url, void *job);
	// 回调:获取登录百度盘的cookie
	static void GetLoginCookieCallBack(wkeWebView webWindow, void* param);
	/*
	切换网盘目录
	*/
	static jsValue SwitchDirPath(jsExecState es, void* param);
	/*打开指定网址*/
	static jsValue OpenAssignUrl(jsExecState es, void* param);
	/*更新应用*/
	static jsValue UpdateApp(jsExecState es, void* param);
	/*检测是否需要更新*/
	static jsValue isUpdate(jsExecState es, void* param);
	/*
	下载用户网盘的文件
	*/
	static jsValue DownloadUserFile(jsExecState es, void* param);
	/*下载分享链接*/
	static jsValue DownShareFile(jsExecState es, void* param);
	/*下载用户选择的分享连接*/
	static jsValue DownSelectShareFile(jsExecState es, void* param);
	/*判断下载分享的链接是什么下载类型
	 1、百度网盘 2、蓝奏网盘 3、城通网盘 0 是无知类型下载地址
	 */
	int JudgeDownUrlType(const std::string& strUrl);
	/*
	组装Aria2下载的json信息
	*/
	std::string CreateDowndAria2Json(REQUESTINFO& Downdinfo);
	//字符串替换
	std::string& replace_all_distinct(std::string& str, const std::string& old_value, const std::string& new_value);
	// 下载百度网盘内的文件
	bool DownloadUserLocalFile(const std::string& strJsonData);
	//解析UI界面传递来的下载文件json 投递给aria2下载
	void DownloadUiShareFile(const std::string & strJson);
	/* 回调：创建新的页面，比如说调用了 window.open 或者点击了 <a target="_blank" .../>*/
   static wkeWebView onCreateView(wkeWebView webWindow, void* param, wkeNavigationType navType, const wkeString url, const wkeWindowFeatures* features);
   /*更新正在下载的列表数据到UI界面*/
   void UpdateDownloadList(const std::string& strJson);
   /*
	分解URL链接地址获取主域名
	*/
	std::string CarkUrl(const char* url);
	/*
	获取资源根目录
	*/
	std::wstring getResourcesPath(const std::wstring name);
	/*
	编码转换
	*/
	std::wstring utf8ToUtf16(const std::string& utf8String);
	/*
	读取本地文件资源
	*/
	void readJsFile(const wchar_t* path, std::vector<char>* buffer);
	/*读取文件*/
	DWORD ReadFileBuffer(std::string szFileName, PVOID* pFileBuffer);
	/*
	@ 初始化miniblink
	*/
	bool InintMiniBlink();
};
#endif

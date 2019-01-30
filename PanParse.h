#ifndef __PANPARSE__
#define __PANPARSE__
#ifndef min
#define min
#endif
#ifndef max
#define max
#endif
#include <atlimage.h>
#undef min
#undef max
#include "Http_Request.h"
#include "rapidjson/document.h"
#define HOME_URL "https://pan.baidu.com/disk/home?#/all?path=%2F&vmode=list"
#define FILE_LIST_URL "https://pan.baidu.com/api/list?order=name&desc=1&showempty=0&web=1&page=1&num=100&dir=%1%&t=0.1312003882264028&channel=chunlei&web=1&app_id=250528&bdstoken=%2%&logid=%3%&clienttype=0&startLogTime=%4%"
#define DOWN_LOCAL_FILE "http://d.pcs.baidu.com/rest/2.0/pcs/file?app_id=250528&channel=00000000000000000000000000000000&check_blue=1&clienttype=8&devuid=0&dtype=1&ehps=0&err_ver=1.0&es=1&esl=1&method=locatedownload&path=%1%&ver=4.0&version=6.0.0.12&vip=3"
#define SHARE_FILE_URL_1 "https://pan.baidu.com/share/set?channel=chunlei&clienttype=0&web=1&channel=chunlei&web=1&app_id=250528&bdstoken=%1%&logid=%2%&clienttype=0"
#define SHARE_FILE_URL_2 "https://pan.baidu.com/share/pset?channel=chunlei&clienttype=0&web=1&channel=chunlei&web=1&app_id=250528&bdstoken=%1%&logid=%2%&clienttype=0"
#define DISK_CAPACITY_QUERY "https://pan.baidu.com/api/quota?app_id=250528&bdstoken=%1%&channel=chunlei&checkexpire=1&checkfree=1&clienttype=0&web=1"
#define NETDISK_USER_AGENT "netdisk;6.0.0.12;PC;PC-Windows;10.0.16299;WindowsBaiduYunGuanJia"
#define SHARE_FILE_LIST_URL "https://pan.baidu.com/share/list?uk=%1%&shareid=%2%&order=other&desc=1&showempty=0&web=1&page=1&num=100&dir=%3%&t=0.46885612477703087&channel=chunlei&web=1&app_id=250528&bdstoken=%4%&logid=%5%&clienttype=0"
#define RENAME_FILE_URL "https://pan.baidu.com/api/filemanager?app_id=250528&async=1&bdstoken=%1%&channel=chunlei&clienttype=0&opera=rename&web=1"
#define OFF_LINE_DOWNLOAD_URL "https://pan.baidu.com/rest/2.0/services/cloud_dl?app_id=250528&bdstoken=%1%&channel=chunlei&clienttype=0&file_sha1=&method=add_task&save_path=%2%&source_url=%3%&type=3&web=1"
#define QUERY_LIST_TASKIDS_URL "https://pan.baidu.com/rest/2.0/services/cloud_dl?app_id=250528&bdstoken=%1%&channel=chunlei&clienttype=0&method=query_task&op_type=1&task_ids=%2%&web=1"
#define OFF_LINE_QUERY_ALL_URL "https://pan.baidu.com/rest/2.0/services/cloud_dl?app_id=250528&bdstoken=%1%&channel=chunlei&clienttype=0&limit=100&method=list_task&need_task_info=1&start=0&status=255&web=1"
#define OFF_LINE_DELETE_TASKID_URL "https://pan.baidu.com/rest/2.0/services/cloud_dl?app_id=250528&bdstoken=%1%&channel=chunlei&clienttype=0&method=delete_task&task_id=%2%&web=1"
extern HINSTANCE g_hInstance;
typedef struct baiduRequestInfo
{
	std::string sign;
	std::string bdstoken;
	std::string timestamp;
	std::string app_id;
	std::string fs_id;
	std::string uk;
	std::string shareid;
	std::string BDCLND;
	std::string server_filename;
	ULONGLONG server_time;
	UINT n_isdir;
	std::string strPath;
	UINT ncategory;
	ULONGLONG nSize;
}BAIDUREQUESTINFO, *PBAIDUREQUESTINFO;
/*
正则取出内容返回
*/
typedef std::vector<std::string> REGEXVALUE;
/*
验证码信息
*/
typedef struct verCodeinfo
{
	std::string image;
	std::string verCode;
}VERCODEINFO;

/*
返回解析出来的真实地址以及文件的名称
*/
typedef struct RequestInfo
{
	std::string strDownloadUrl;
	std::string strFileName;
	std::string strCookies;
	std::string strSavePath;
	std::string strUserAegnt;
}REQUESTINFO;
/*
获取用户登陆后的基础信息
*/
typedef struct userInfo
{
	std::string strUserName;
	std::string strHeadImageUrl;
	std::string bdstoken;
	std::string strDisktotal;
	std::string strDiskUsed;
	int is_vip;
}BaiduUserInfo;
/*
百度盘用户文件列表属性
*/
typedef struct fileInfo
{
	int nCategory;	//文件类型 6是文件夹或者zip包或者其他类型的文件需要根据isdir判断是否是文件夹 3是图片 2是音乐 1是视频 5是exe文件
	std::string strPath; //文件所在的路径
	std::string server_filename; //文件的名称
	int nisdir;		//是否是文件夹 1是 0 不是
	ULONG size;		//文件的大小
	std::string strFileType;	//根据后缀名来区分文件的类型
	ULONG server_ctime;		//文件的时间戳
	std::string fs_id;		//用作下载使用
}BaiduFileInfo;
/*
文件信息列表数组
*/
typedef std::vector<BaiduFileInfo> FileTypeArray;
typedef struct DateTime
{
	UINT nYear;
	UINT nMonth;
	UINT nDay;
}DATETIME;
/*
分享文件信息
*/
typedef struct shareInfo
{
	std::string strPwd;
	std::string strpath_list;
	int nschannel;
	std::string strperiod;
}SHAREFILEINFO;
class CBaiduParse
{
public:
	explicit CBaiduParse();
	~CBaiduParse();
private:
	CBaiduParse(const CBaiduParse&);
	//验证码请求网址
	static std::string m_vcCodeUrl;
	//验证码
	static std::string m_VerCode;
	//访问密码
	static std::string m_SharePass;
	//验证码窗口回调
	static INT_PTR CALLBACK ImageProc(
		_In_ HWND   hwndDlg,
		_In_ UINT   uMsg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam
		);
	//加载图片
	static BOOL LodcomImage(LPVOID PmemIm, ULONG dwLen, CImage& ImgObj);
	//显示输入验证码的窗口
	void ShowInputVerCodeDlg();
	//访问密码窗口回调
	static INT_PTR CALLBACK ShareProc(
		_In_ HWND   hwndDlg,
		_In_ UINT   uMsg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam
	);
	//显示输入访问密码的窗口
	void ShowInputPassDlg();
public:
/*
获取百度网盘文件列表信息
*/
std::string GetBaiduFileListInfo(const std::string& path, const std::string strCookie);
/*枚举出百度网盘所有文件夹*/
bool EnumAllFolder(const std::string& path, const std::string strCookie, rapidjson::Document& datajson);
/*获取百度网盘分享文件列表信息*/
std::string GetBaiduShareFileListInfo(const std::string& path, const std::string strCookie, BAIDUREQUESTINFO userinfo);
/*
写文件
*/
DWORD WriteFileBuffer(std::string szFileNmae, PVOID pFileBuffer, DWORD dwFileSize);
/*
获取用户登录后的基础信息
*/
bool GetloginBassInfo(BaiduUserInfo& baseInfo,const std::string strCookie);
/*
正则取出指定内容
*/
REGEXVALUE GetRegexValue(const std::string strvalue, const std::string strRegx);
/*
	解析百度盘真实下载地址
	strUrl 请求的网址
	strCookies 登录后的cookies
	返回解析出来的真实地址以及文件的名称
*/
REQUESTINFO ParseBaiduAddr(const std::string strUrl, std::string& strCookies);
/*
解析百度盘真实下载地址
strUrl 请求的网址
strCookies 登录后的cookies
返回解析出来的真实地址以及文件的名称
*/
REQUESTINFO ParseBaiduAddrEx(BAIDUREQUESTINFO& BaiduInfo, std::string& strCookies);
/*
分析用户提供的分享下载链接的文件信息生成json数据回传给UI界面渲染
*/
std::string AnalysisShareUrlInfo(const std::string strUrl,std::string& strCookie);
/*
判断是否是带密码分享链接
*/
int IsPassWordShareUrl(std::string& strUrl, std::string& strCookies);
/*添加离线下载*/
std::string AddOfflineDownload(const std::string& strUrl, const std::string& strSavePath,const std::string& strCookie);
/*查询所有的离线下载任务列表*/
REGEXVALUE QueryOffLineList(const std::string& strCookie);
/*根据taskid查询对应的任务状态列表信息*/
std::string QueryTaskIdListStatus(const REGEXVALUE& TaskIdList, const std::string& strCookie);
/*清除离线下载任务记录*/
std::string DeleteOffLineTask(const REGEXVALUE& TaskIdList, const std::string& strCookie);
/*
带验证码的请求
*/
std::string GetBaiduAddr(BAIDUREQUESTINFO baiduinfo, const std::string strCookies);
/*
获取验证码信息
*/
VERCODEINFO GetVerCodeinfo(BAIDUREQUESTINFO baiduinfo, const std::string strCookies);
/*
获取 BAIDUREQUESTINFO 信息
*/
BAIDUREQUESTINFO GetBaiduInfo(const std::string strJson);
/*
根据文件路径获取需要的下载的真实下载地址
*/
std::string GetBaiduLocalFileAddr(const std::string path, const std::string strCookie);
/*
分享百度用户的文件或者文件夹
*/
std::string ShareBaiduFile(SHAREFILEINFO shareFileinfo, const std::string strCookie);
/*删除百度云文件*/
std::string DeleteBaiduFile(const std::string strJsonData, const std::string strCookie);
/*百度云用户文件重命名*/
std::string BaiduRename(const std::string& strPath,const std::string& strNewName, const std::string strCookie);
/*
 获取时间戳
*/
long long getTimeStamp();
/*
取文本中间内容
*/
std::string GetTextMid(const std::string strSource, const std::string strLeft, const std::string strRight);
/*
获取请求需要的Logid
*/
std::string GetLogid(bool isFlag = true);
inline float roundEx(float Floatnum);
/*
获取文件大小类型
*/
std::string GetFileSizeType(double dSize);
/*
时间戳转日期
*/
std::string timestampToDate(ULONGLONG ctime);
/*
分割日期函数
*/
DATETIME GetDateTime(const std::string& strTime);
/*
编码转换
*/
std::string Unicode_To_Ansi(const wchar_t* szbuff);
std::wstring Ansi_To_Unicode(const char* szbuff);
std::string Gbk_To_Utf8(const char* szBuff);
std::string Utf8_To_Gbk(const char* szBuff);
// 参数 szSource 需要被URL编码的字符串
//参数 isletter 是否不编码数字字母 默认为true 不编码 false 则编码
//参数 isUtf8 是否编码为UTF-8格式 默认为true UTF-8编码 false 则不编码为utf-8
/***************************************************/
std::string URL_Coding(const char* szSource, bool isletter = true, bool isUtf8 = true);
//解析\uxxxx\uxxxx编码字符
std::string UnEscape(const char* strSource);

};
#endif

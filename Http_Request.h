#ifndef __HTTP_REQUEST__
#define __HTTP_REQUEST__
#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <curl/curl.h>
#define USER_AGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:64.0) Gecko/20100101 Firefox/64.0"
/*
 用来操作HTTP协议请求
 核心使用libcurl
*/
typedef void* RequestResultPtr;
typedef std::vector<char> responseData;
/*
http 请求的类型
*/
typedef enum EnumRequestType
{
	GET,
	POST,
	HEAD
}RequestType;
/*
设置请求协议头的map类型
*/
typedef std::map<std::string, std::string> RequestHeaderValue;
class HttpRequest
{
public:
	HttpRequest();
	~HttpRequest();
	/*
	设置请求的协议头 以KEY value 方式设置
	如key = "user-agent"
	value = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/69.0.3497.100 Safari/537.36"
	*/
	void SetRequestHeader(const std::string strKey, const std::string strValue);
	/*
	设置请求的协议头 以map类型方式设置
	*/
	void SetRequestHeader(RequestHeaderValue& HeaderValue);
	/*
	发送HTTP请求
	Enumtype http 请求的类型
	strPost 当 Enumtype =POST 时才有用
	*/
	void Send(RequestType Enumtype,const std::string strUrl,const std::string strPost="");
	/*
	获取获取http请求响应的网页的内容
	*/
	std::string GetResponseText();
	/*
	获取获取http请求响应的网页的二进制数据 如image File等
	*/
	responseData GetResponseBody() const;
	/*
	获取http请求响应的头部分
	*/
	std::string GetallResponseHeaders();
	/*
	设置请求用到的cookie
	*/
	void SetRequestCookies(const std::string strCookie);
	/*
	开启或者禁止重定向
	*/
	void SetHttpRedirect(bool val);
	/*
	 获取本次请求的返回的cookies
	*/
	std::string GetResponCookie() const;
	/*
	合并cookies
	*/
	std::string MergeCookie(const std::string wornCookie, const std::string newCookie);
private:
	/*以下为内部使用函数*/
	/*
	自动补齐请求协议头
	*/
	void AutoAddHeader();
	/*
	取返回协议头cookies
	*/
	std::string GetCookies(const std::string strHeader);
	/*
	合并cookie内部处理
	*/
	RequestHeaderValue SplitCookie(const std::string strCookie);
	/*
	字符串分割
	*/
	std::vector<std::string> StrSplit(const std::string &str, const std::string &pattern);
	/*
	查看自定义请求协议头是否存在
	*/
	bool CheckHeaderExist(const std::string strHeaderKey);
	static size_t header_callback(char *ptr, size_t size, size_t nmemb, void* userdata);
	static size_t read_callback(char *ptr, size_t size, size_t nmemb, void* userdata);
private:
	/*
	CURL句柄
	*/
	CURL* m_CurlHandle;
	/*
	 用来接收请求的结果
	*/
	responseData m_Request;
	/*
	用来接收返回的协议头
	*/
	responseData m_ResponseHeader;
	/*
	协议头链表指针
	*/
	struct curl_slist* headerList;
	/*
	内部处理cookie 用户合并更新cookies使用
	*/
	std::string m_Cookies;
};
#endif


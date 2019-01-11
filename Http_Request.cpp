#include "Http_Request.h"
#include <set>
#include <stdexcept>
#ifdef _DEBUG
#pragma comment(lib,"libcurld.lib")
#else
#pragma comment(lib,"libcurl.lib")
#endif
HttpRequest::HttpRequest()
:m_CurlHandle(NULL),
headerList(nullptr),
m_Cookies("")
{
	try{
		CURLcode rcode = curl_global_init(CURL_GLOBAL_ALL);
		if (rcode)
			throw rcode;
		m_CurlHandle = curl_easy_init();
		if (m_CurlHandle)
		{
			curl_easy_setopt(m_CurlHandle, CURLOPT_SSL_VERIFYPEER, false);	// 验证对方的SSL证书
			curl_easy_setopt(m_CurlHandle, CURLOPT_SSL_VERIFYHOST, false);	//根据主机验证证书的名称
		}
	}
	catch (CURLcode& errCode)
	{
		//输出日志
		throw std::runtime_error("LibCurl initialization environment failed");
	}
}

HttpRequest::~HttpRequest()
{
	if (headerList)
	{
		curl_slist_free_all(headerList);
		headerList = nullptr;
	}
	if (m_CurlHandle)
	{
		curl_easy_cleanup(m_CurlHandle);
		m_CurlHandle = nullptr;
	}
	curl_global_cleanup();	//关闭curl环境
}

void HttpRequest::SetRequestHeader(const std::string strKey, const std::string strValue)
{
	std::string strRequestHeader(strKey + ":" + strValue);
	headerList = curl_slist_append(headerList, strRequestHeader.c_str());
}

void HttpRequest::SetRequestHeader(RequestHeaderValue& HeaderValue)
{
	for (auto &v : HeaderValue)
	{
		std::string strRequestHeader(v.first + ":" + v.second);
		headerList = curl_slist_append(headerList, strRequestHeader.c_str());
	}
}

void HttpRequest::Send(RequestType Enumtype, const std::string strUrl,const std::string strPost)
{
	CURLcode dwCurlCode;
	if (!m_CurlHandle)return;
	m_Request.clear();
	m_ResponseHeader.clear();
	m_Cookies.clear();
	switch (Enumtype)
	{
	case GET:
	{
				AutoAddHeader();
				if (!CheckHeaderExist("Referer:"))
				{
					headerList = curl_slist_append(headerList, ("Referer:" + strUrl).c_str());
				}
				//设置URL
				curl_easy_setopt(m_CurlHandle, CURLOPT_URL, strUrl.c_str());
				curl_easy_setopt(m_CurlHandle, CURLOPT_HTTPHEADER, headerList);	//设置自定义协议头头
				curl_easy_setopt(m_CurlHandle, CURLOPT_HEADERFUNCTION, header_callback);	//设置回调协议头函数
				curl_easy_setopt(m_CurlHandle, CURLOPT_HEADERDATA, &m_ResponseHeader);
				curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEFUNCTION, read_callback);  //设置写的回调函数
				curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEDATA, &m_Request);	//接收返回的数据
				curl_easy_setopt(m_CurlHandle, CURLOPT_NOBODY, 0);	//接收返回的数据
				dwCurlCode = curl_easy_perform(m_CurlHandle);
				if (CURLE_OK != dwCurlCode)
				{
					//输出日志
				}
				m_Cookies = GetCookies(GetallResponseHeaders());

	}
	break;
	case POST:
	{
				 AutoAddHeader();
				 if (!CheckHeaderExist("Referer:"))
				 {
					 headerList = curl_slist_append(headerList, ("Referer:" + strUrl).c_str());
				 }
				 if (!CheckHeaderExist("Content-Type:"))
				 {
					 headerList = curl_slist_append(headerList, ("Content-Type: application/x-www-form-urlencoded"));
				 }
				 //设置URL
				 curl_easy_setopt(m_CurlHandle, CURLOPT_URL, strUrl.c_str());
				 curl_easy_setopt(m_CurlHandle, CURLOPT_HTTPHEADER, headerList);	//设置自定义协议头头
				 curl_easy_setopt(m_CurlHandle, CURLOPT_POSTFIELDSIZE, (long)strPost.length());	//设置需要发送的数据大小
				 curl_easy_setopt(m_CurlHandle, CURLOPT_POSTFIELDS, strPost.c_str());	//设置需要发送的数据
				 curl_easy_setopt(m_CurlHandle, CURLOPT_HEADERFUNCTION, header_callback);	//设置回调协议头函数
				 curl_easy_setopt(m_CurlHandle, CURLOPT_HEADERDATA, &m_ResponseHeader);
				 curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEFUNCTION, read_callback);  //设置写的回调函数
				 curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEDATA, &m_Request);	//接收返回的数据
				 curl_easy_setopt(m_CurlHandle, CURLOPT_NOBODY, 0);	//接收返回的数据
				 dwCurlCode = curl_easy_perform(m_CurlHandle);
				 if (CURLE_OK != dwCurlCode)
				 {
					 //输出日志
				 }
				 m_Cookies = GetCookies(GetallResponseHeaders());
	}
	break;
	case HEAD:
	{
				 AutoAddHeader();
				 if (!CheckHeaderExist("Referer:"))
				 {
					 headerList = curl_slist_append(headerList, ("Referer:" + strUrl).c_str());
				 }
				 //设置URL
				 curl_easy_setopt(m_CurlHandle, CURLOPT_URL, strUrl.c_str());
				 curl_easy_setopt(m_CurlHandle, CURLOPT_HTTPHEADER, headerList);	//设置自定义协议头头
				 curl_easy_setopt(m_CurlHandle, CURLOPT_HEADERFUNCTION, header_callback);	//设置回调协议头函数
				 curl_easy_setopt(m_CurlHandle, CURLOPT_HEADERDATA, &m_ResponseHeader);
				 curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEFUNCTION, read_callback);  //设置写的回调函数
				 curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEDATA, &m_Request);	//接收返回的数据
				 curl_easy_setopt(m_CurlHandle, CURLOPT_NOBODY, 1);	//接收返回的数据
				 dwCurlCode = curl_easy_perform(m_CurlHandle);
				 if (CURLE_OK != dwCurlCode)
				 {
					 //输出日志
				 }
				 m_Cookies = GetCookies(GetallResponseHeaders());
	}
	default:
		break;
	}
	if (headerList)
	{
		curl_slist_free_all(headerList);
		headerList = nullptr;
	}
}

std::string HttpRequest::GetResponseText()
{
	std::string strResultText;
	for (auto v: m_Request)
		strResultText.append(1, v);
	return strResultText;
}

responseData HttpRequest::GetResponseBody() const
{
	return m_Request;
}

std::string HttpRequest::GetallResponseHeaders()
{
	std::string strResultText;
	for (auto v : m_ResponseHeader)
		strResultText.append(1, v);
	return strResultText;
}

void HttpRequest::SetRequestCookies(const std::string strCookie)
{
	if (!m_CurlHandle)return;
	curl_easy_setopt(m_CurlHandle, CURLOPT_COOKIE, strCookie.c_str());
	
}

void HttpRequest::SetHttpRedirect(bool val)
{
	if (!m_CurlHandle)return;
	curl_easy_setopt(m_CurlHandle, CURLOPT_FOLLOWLOCATION, (long)val);
}

std::string HttpRequest::GetResponCookie() const
{
	return m_Cookies;
}

void HttpRequest::AutoAddHeader()
{
	if (!headerList || !CheckHeaderExist("Accept:"))
	{
		headerList = curl_slist_append(headerList, "Accept:*/*");
	}
	if (!CheckHeaderExist("Accept-Language:"))
	{
		headerList = curl_slist_append(headerList, "Accept-Language:zh-cn");
	}
	if (!CheckHeaderExist("User-Agent:"))
	{
		headerList = curl_slist_append(headerList, "User-Agent:Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/69.0.3497.100 Safari/537.36");
	}
}

std::string HttpRequest::GetCookies(const std::string strHeader)
{
	std::string strResult, strTwoResult;
	std::vector<std::string> headerList;
	std::set<std::string> resultList;
	if (!strHeader.empty())
	{
		std::string split = strHeader + "\r\n";
		std::string strsub;
		size_t pos = 0;
		size_t npos = 0;
		npos = split.find("\r\n", pos);
		while (npos != std::string::npos)
		{
			strsub = split.substr(pos, npos - pos);
			pos = npos + strlen("\r\n");
			npos = split.find("\r\n", pos);
			headerList.push_back(strsub);
		}
	}
	for (auto & v : headerList)
	{
		std::string strTmp;
		int nPos = v.find("Set-Cookie:");
		if (nPos != std::string::npos)
		{
			std::string strTmp = v.substr(nPos + lstrlenA("Set-Cookie:"), v.length() - (nPos + lstrlenA("Set-Cookie:")));;
			if (strTmp.at(strTmp.length() - 1) != ';')
			{
				strTmp += ";";
			}
			strResult += strTmp;
		}
	}
	headerList.clear();
	if (!strResult.empty())
	{
		if (strResult.at(strResult.length() - 1) != ';')
			strResult += ";";
		std::string split = strResult;
		std::string strsub;
		size_t pos = 0;
		size_t npos = 0;
		npos = split.find(";", pos);
		while (npos != std::string::npos)
		{
			strsub = split.substr(pos, npos - pos);
			pos = npos + strlen(";");
			npos = split.find(";", pos);
			resultList.insert(strsub);
		}
	}
	else
	{
		return strResult;
	}

	for (auto& s : resultList)
		strTwoResult += s + ";";
	return strTwoResult;
}

std::vector<std::string> HttpRequest::StrSplit(const std::string &str, const std::string &pattern)
{
	std::vector<std::string> resultVec;
	std::string split = str;
	std::string strsub;
	size_t pos = 0;
	size_t npos = 0;
	npos = split.find(pattern, pos);
	while (npos != std::string::npos)
	{
		strsub = split.substr(pos, npos - pos);
		pos = npos + pattern.length();
		npos = split.find(pattern, pos);
		resultVec.push_back(strsub);
	}
	return resultVec;
}

std::string HttpRequest::MergeCookie(const std::string wornCookie, const std::string newCookie)
{
	std::string strResult = wornCookie;
	std::vector<std::string> strCookieArr = StrSplit(newCookie, ";");
	for (auto v : strCookieArr)
	{
		int npos = std::string::npos;
		if (strResult.find(v) == npos)
			strResult += v + ";";
	}
	return strResult;
}

bool HttpRequest::CheckHeaderExist(const std::string strHeaderKey)
{
	bool isBresult = false;
	struct curl_slist* TmpheaderList = headerList;
	while (TmpheaderList)
	{
		std::string tmpHeaderstr = TmpheaderList->data;
		if (tmpHeaderstr.find(strHeaderKey)!=std::string::npos)
		{
			isBresult = true;
			break;
		}
		TmpheaderList = TmpheaderList->next;
	}
	return isBresult;
}

size_t HttpRequest::header_callback(char *ptr, size_t size, size_t nmemb, void* userdata)
{
	size_t lsize = size * nmemb;
	for (size_t i = 0; i < lsize; i++)
		((responseData*)userdata)->push_back(ptr[i]);
	return lsize;
}

size_t HttpRequest::read_callback(char *ptr, size_t size, size_t nmemb, void* userdata)
{
	size_t lsize = size * nmemb;
	for (size_t i = 0; i < lsize; i++)
		((responseData*)userdata)->push_back(ptr[i]);
	return lsize;
}


#include "PanParse.h"
#include <sys/timeb.h>
#include <dbghelp.h>
#include <shlwapi.h>
#include <boost/regex.hpp>
#include <boost/format.hpp>
#include "base64.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "resource.h"
#pragma comment(lib,"dbghelp.lib")
#pragma comment(lib,"shlwapi.lib")
std::string CBaiduParse::m_vcCodeUrl;
std::string CBaiduParse::m_VerCode;
DWORD CBaiduParse::WriteFileBuffer(std::string szFileNmae, PVOID pFileBuffer, DWORD dwFileSize)
{
	DWORD dwStates = -1;
	std::string szFilePath, strName;
	size_t npos = -1;
	if (szFileNmae.empty())
		return dwStates;
	npos = szFileNmae.rfind("\\");
	if (npos != std::string::npos)
	{
		szFilePath = szFileNmae.substr(0, npos + 1);
		if (!szFilePath.empty())
		{
			if (MakeSureDirectoryPathExists(szFilePath.c_str()))
			{
				HANDLE hFile = INVALID_HANDLE_VALUE;
				DWORD dwError = NULL;
				hFile = CreateFileA(szFileNmae.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				dwError = ::GetLastError();
				if (hFile != INVALID_HANDLE_VALUE)
				{
					::WriteFile(hFile, pFileBuffer, dwFileSize, &dwStates, NULL);
					/*	CloseHandle(hFile);
					ofstream file(szFileNmae, std::ios::out|std::ios::binary);
					//file.open(szFileNmae,std::ios::out);
					if (!file.is_open())
					return dwStates;
					file.write((char*)pFileBuffer, dwFileSize);
					file.close();
					dwStates = NULL;*/
					::CloseHandle(hFile);
				}
			}
		}
	}

	return dwStates;
}
REQUESTINFO CBaiduParse::ParseBaiduAddr(const std::string strUrl, std::string& strCookies)
{
	HttpRequest BaiduHttp;
	BAIDUREQUESTINFO BaiduInfo;
	REQUESTINFO strRealUrl;
	std::string strDownloadUrl;
	std::string strResponseText;
	BaiduHttp.SetRequestCookies(strCookies);
	BaiduHttp.SetHttpRedirect(true);
	BaiduHttp.Send(GET, strUrl);
	strResponseText = BaiduHttp.GetResponseText();
	strResponseText = Utf8_To_Gbk(strResponseText.c_str());
	if (strResponseText.empty())
		return strRealUrl;
	std::string strJson = GetTextMid(strResponseText, "yunData.setData(", ");");
	BaiduInfo = GetBaiduInfo(strJson);
	strDownloadUrl = "https://pan.baidu.com/api/sharedownload?sign=" + BaiduInfo.sign + \
		"&timestamp=" + BaiduInfo.timestamp + "&channel=chunlei&web=1&app_id=" + BaiduInfo.app_id + \
		"&bdstoken=" + BaiduInfo.bdstoken + "&logid=" + GetLogid() + "&clienttype=0";
	strCookies = BaiduHttp.MergeCookie(strCookies, BaiduHttp.GetResponCookie());
	BaiduInfo.BDCLND = GetTextMid(strCookies, "BDCLND=", ";");
	if (BaiduInfo.BDCLND.empty())
	{
		strJson = "encrypt=0&product=share&uk=%1%&primaryid=%2%&fid_list=%%5B%3%%%5D&path_list=";
		strJson = str(boost::format(strJson) % BaiduInfo.uk % BaiduInfo.shareid % BaiduInfo.fs_id);
	}
	else
	{
		strJson = "encrypt=0&extra=%%7B%%22sekey%%22%%3A%%22%1%%%22%%7D&product=share&uk=%2%&primaryid=%3%&fid_list=%%5B%4%%%5D&path_list=";
		strJson = str(boost::format(strJson) % BaiduInfo.BDCLND % BaiduInfo.uk % BaiduInfo.shareid % BaiduInfo.fs_id);
	}
	/*准备提交数据获取真实URL地址*/
	BaiduHttp.SetRequestHeader("Content-Type", " application/x-www-form-urlencoded; charset=UTF-8");
	BaiduHttp.SetRequestHeader("Accept", " application/json, text/javascript, */*; q=0.01");
	BaiduHttp.SetRequestCookies(strCookies);
	BaiduHttp.Send(POST, strDownloadUrl, strJson);
	strJson = BaiduHttp.GetResponseText();
	rapidjson::Document dc;
	dc.Parse(strJson.c_str());
	if (!dc.IsObject())
		return strRealUrl;
	int nError = -1;
	if (dc.HasMember("errno") && dc["errno"].IsInt())
		nError = dc["errno"].GetInt();
	if (!nError)
	{
		if (dc.HasMember("list") && dc["list"].IsArray())
		{
			for (auto &v : dc["list"].GetArray())
			{
				if (v.HasMember("dlink") && v["dlink"].IsString())
					strRealUrl.strDownloadUrl = v["dlink"].GetString();
			}
		}
	}
	else
	{
		//这里需要输入验证码了
		int index = 0;
		do 
		{
			if (index>5)
				break;
			strRealUrl.strDownloadUrl = GetBaiduAddr(BaiduInfo, strCookies);
			index++;
		} while (strRealUrl.strDownloadUrl.empty());
	}
	BaiduHttp.Send(HEAD, strRealUrl.strDownloadUrl);
	strRealUrl.strDownloadUrl = GetTextMid(BaiduHttp.GetallResponseHeaders(), "Location: ", "\r\n");
	strRealUrl.strFileName = BaiduInfo.server_filename;
	strRealUrl.strCookies = strCookies;
	return strRealUrl;
}
std::string CBaiduParse::GetBaiduAddr(BAIDUREQUESTINFO baiduinfo, const std::string strCookies)
{

	std::string strRealUrl;
	std::string strVcCode;
	VERCODEINFO verCinfo = GetVerCodeinfo(baiduinfo, strCookies);
	m_vcCodeUrl = verCinfo.image;
	std::string strJson;
	//显示验证码输入窗口
	ShowInputVerCodeDlg();
	HttpRequest BaiduHttp;
	strVcCode = m_VerCode;
	std::string strDownloadUrl = "https://pan.baidu.com/api/sharedownload?sign=" + baiduinfo.sign + \
		"&timestamp=" + baiduinfo.timestamp + "&channel=chunlei&web=1&app_id=" + baiduinfo.app_id + \
		"&bdstoken=" + baiduinfo.bdstoken + "&logid=" + GetLogid() + "&clienttype=0";
	baiduinfo.BDCLND = GetTextMid(strCookies, "BDCLND=", ";");
	if (baiduinfo.BDCLND.empty())
	{
		strJson = "encrypt=0&product=share&vcode_input=%1%&vcode_str=%2%&uk=%3%&primaryid=%4%&fid_list=%%5B%5%%%5D";
		strJson = str(boost::format(strJson) % strVcCode % verCinfo.verCode % baiduinfo.uk % baiduinfo.shareid % baiduinfo.fs_id);
	}
	else
	{
		strJson = "encrypt=0&extra=%%7B%%22sekey%%22%%3A%%22%1%%%22%%7D&product=share&vcode_input=%2%&vcode_str=%3%&uk=%4%&primaryid=%5%&fid_list=%%5B%6%%%5D&path_list=";
		strJson = str(boost::format(strJson) % baiduinfo.BDCLND % strVcCode % verCinfo.verCode % baiduinfo.uk % baiduinfo.shareid % baiduinfo.fs_id);
	}
	/*准备提交数据获取真实URL地址*/
	BaiduHttp.SetRequestHeader("Content-Type", " application/x-www-form-urlencoded; charset=UTF-8");
	BaiduHttp.SetRequestHeader("Accept", " application/json, text/javascript, */*; q=0.01");
	BaiduHttp.SetRequestCookies(strCookies);
	BaiduHttp.Send(POST, strDownloadUrl, strJson);
	strJson = BaiduHttp.GetResponseText();
	rapidjson::Document dc;
	dc.Parse(strJson.c_str());
	if (!dc.IsObject())
		return strRealUrl;
	int nError = -1;
	if (dc.HasMember("errno") && dc["errno"].IsInt())
		nError = dc["errno"].GetInt();
	if (!nError)
	{
		if (dc.HasMember("list") && dc["list"].IsArray())
		{
			for (auto &v : dc["list"].GetArray())
			{
				if (v.HasMember("dlink") && v["dlink"].IsString())
					strRealUrl = v["dlink"].GetString();
			}
		}
	}
	return strRealUrl;
}
VERCODEINFO CBaiduParse::GetVerCodeinfo(BAIDUREQUESTINFO baiduinfo, const std::string strCookies)
{
	VERCODEINFO result;
	HttpRequest BaiduHttp;
	std::string strJson;
	std::string strVerCodeUrl = "https://pan.baidu.com/api/getvcode?prod=pan&t=0.48382518029895166&channel=chunlei&web=1&app_id=" + baiduinfo.app_id + "&bdstoken=" + baiduinfo.bdstoken + "&logid=" + GetLogid() + "&clienttype=0";
	BaiduHttp.SetRequestHeader("Accept", " application/json, text/javascript, */*; q=0.01");
	BaiduHttp.SetRequestHeader("Accept-Language", "zh-Hans-CN,zh-Hans;q=0.8,en-US;q=0.5,en;q=0.3");
	BaiduHttp.SetRequestCookies(strCookies);
	BaiduHttp.Send(GET, strVerCodeUrl);
	strJson = BaiduHttp.GetResponseText();
	rapidjson::Document dc;
	dc.Parse(strJson.c_str());
	if (!dc.IsObject())
		return result;
	if (dc.HasMember("img") && dc["img"].IsString())
		result.image = dc["img"].GetString();
	if (dc.HasMember("vcode") && dc["vcode"].IsString())
		result.verCode = dc["vcode"].GetString();
	return result;
}

long long CBaiduParse::getTimeStamp()
{
	timeb t;
	ftime(&t);
	return t.time * 1000 + t.millitm;
}

BAIDUREQUESTINFO CBaiduParse::GetBaiduInfo(const std::string strJson)
{
	BAIDUREQUESTINFO baiduInfo;
	rapidjson::Document dc;
	dc.Parse(strJson.c_str());
	if (!dc.IsObject())
		return baiduInfo;
	if (dc.HasMember("bdstoken") && dc["bdstoken"].IsString())
		baiduInfo.bdstoken = dc["bdstoken"].GetString();
	if (dc.HasMember("shareid") && dc["shareid"].IsInt64())
		baiduInfo.shareid = std::to_string(dc["shareid"].GetInt64());
	if (dc.HasMember("uk") && dc["uk"].IsUint64())
		baiduInfo.uk = std::to_string(dc["uk"].GetUint64());
	if (dc.HasMember("sign") && dc["sign"].IsString())
		baiduInfo.sign = dc["sign"].GetString();
	if (dc.HasMember("timestamp") && dc["timestamp"].IsInt())
		baiduInfo.timestamp = std::to_string(dc["timestamp"].GetInt());
	if (dc.HasMember("file_list") && dc["file_list"].IsObject())
	{
		rapidjson::Value file_list = dc["file_list"].GetObject();
		if (file_list.HasMember("list") && file_list["list"].IsArray())
		{
			for (auto& v : file_list["list"].GetArray())
			{
				if (v.HasMember("app_id") && v["app_id"].IsString())
				{
					baiduInfo.app_id = v["app_id"].GetString();
				}
				if (v.HasMember("server_filename") && v["server_filename"].IsString())
				{
					baiduInfo.server_filename = v["server_filename"].GetString();
				}
				if (v.HasMember("server_ctime") && v["server_ctime"].IsUint64())
				{
					baiduInfo.server_time = v["server_ctime"].GetUint64();
				}
				if (v.HasMember("fs_id") && v["fs_id"].IsUint64())
				{
					baiduInfo.fs_id = std::to_string(v["fs_id"].GetUint64());
				}
			}
		}
	}
	return baiduInfo;
}

std::string CBaiduParse::Unicode_To_Ansi(const wchar_t* szbuff)
{
	std::string result;
	CHAR* MultPtr = nullptr;
	int MultLen = -1;
	MultLen = ::WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, szbuff, -1, NULL, NULL, NULL, NULL);
	MultPtr = new CHAR[MultLen + 1];
	if (MultPtr)
	{
		ZeroMemory(MultPtr, MultLen + 1);
		::WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, szbuff, -1, MultPtr, MultLen, NULL, NULL);
		result = MultPtr;
		delete[] MultPtr;
		MultPtr = nullptr;
	}
	return result;
}

std::wstring CBaiduParse::Ansi_To_Unicode(const char* szbuff)
{
	std::wstring result;
	WCHAR* widePtr = nullptr;
	int wideLen = -1;
	wideLen = ::MultiByteToWideChar(CP_ACP, NULL, szbuff, -1, NULL, NULL);
	widePtr = new WCHAR[wideLen + 1];
	if (widePtr)
	{
		ZeroMemory(widePtr, (wideLen + 1) * sizeof(WCHAR));
		::MultiByteToWideChar(CP_ACP, NULL, szbuff, -1, widePtr, wideLen);
		result = widePtr;
		delete[] widePtr;
		widePtr = nullptr;
	}
	return result;
}

std::string CBaiduParse::Gbk_To_Utf8(const char* szBuff)
{
	std::string resault;
	int widLen = 0;
	int MultiLen = 0;
	WCHAR* widPtr = nullptr;
	CHAR* MulitPtr = nullptr;
	widLen = ::MultiByteToWideChar(CP_ACP, NULL, szBuff, -1, NULL, NULL);//获取转换后需要的空间
	widPtr = new WCHAR[widLen + 1];
	if (!widPtr)
		return resault;
	ZeroMemory(widPtr, (widLen + 1) * sizeof(WCHAR));
	if (!::MultiByteToWideChar(CP_ACP, NULL, szBuff, -1, widPtr, widLen))
	{
		delete[] widPtr;
		widPtr = nullptr;
		return resault;
	}
	MultiLen = ::WideCharToMultiByte(CP_UTF8, NULL, widPtr, -1, NULL, NULL, NULL, NULL);
	if (MultiLen)
	{
		MulitPtr = new CHAR[MultiLen + 1];
		if (!MulitPtr)
		{
			delete[] widPtr;
			widPtr = nullptr;
			return resault;
		}
		ZeroMemory(MulitPtr, (MultiLen + 1) * sizeof(CHAR));
		::WideCharToMultiByte(CP_UTF8, NULL, widPtr, -1, MulitPtr, MultiLen, NULL, NULL);
		resault = MulitPtr;
		delete[] MulitPtr;
		MulitPtr = nullptr;
	}
	delete[] widPtr;
	widPtr = nullptr;
	return resault;
}

std::string CBaiduParse::Utf8_To_Gbk(const char* szBuff)
{
	std::string resault;
	int widLen = 0;
	int MultiLen = 0;
	WCHAR* widPtr = nullptr;
	CHAR* MulitPtr = nullptr;
	widLen = ::MultiByteToWideChar(CP_UTF8, NULL, szBuff, -1, NULL, NULL);//获取转换后需要的空间
	widPtr = new WCHAR[widLen + 1];
	if (!widPtr)
		return resault;
	ZeroMemory(widPtr, (widLen + 1) * sizeof(WCHAR));
	if (!::MultiByteToWideChar(CP_UTF8, NULL, szBuff, -1, widPtr, widLen))
	{
		delete[] widPtr;
		widPtr = nullptr;
		return resault;
	}
	MultiLen = ::WideCharToMultiByte(CP_ACP, NULL, widPtr, -1, NULL, NULL, NULL, NULL);
	if (MultiLen)
	{
		MulitPtr = new CHAR[MultiLen + 1];
		if (!MulitPtr)
		{
			delete[] widPtr;
			widPtr = nullptr;
			return resault;
		}
		ZeroMemory(MulitPtr, (MultiLen + 1) * sizeof(CHAR));
		::WideCharToMultiByte(CP_ACP, NULL, widPtr, -1, MulitPtr, MultiLen, NULL, NULL);
		resault = MulitPtr;
		delete[] MulitPtr;
		MulitPtr = nullptr;
	}
	delete[] widPtr;
	widPtr = nullptr;
	return resault;
}


std::string CBaiduParse::GetTextMid(const std::string strSource, const std::string strLeft, const std::string strRight)
{
	std::string strResult;
	int nBeigin = 0, nEnd = 0, nLeft = 0;
	if (strSource.empty())return strResult;
	nBeigin = strSource.find(strLeft);
	if (nBeigin == std::string::npos)return strResult;
	nLeft = nBeigin + strLeft.length();
	nEnd = strSource.find(strRight, nLeft);
	if (nEnd == std::string::npos)
	{
		strResult = strSource.substr(nLeft, strSource.length());
		return strResult;
	}
	strResult = strSource.substr(nLeft, nEnd - nLeft);
	return strResult;
}

std::string CBaiduParse::GetLogid(bool isFlag /*= true*/)
{
	std::string strResult;
	srand(ULLONG_MAX);
	CHAR szLogid[0x20];
	CHAR szTime[0x20];
	CHAR szResult[0x50];
	ZeroMemory(szLogid, 0x20);
	ZeroMemory(szTime, 0X20);
	ZeroMemory(szResult, 0X50);
	sprintf_s(szLogid, "%I64u", labs(rand()));
	sprintf_s(szTime, "%I64d", getTimeStamp());
	strcat_s(szResult, szTime);
	strcat_s(szResult, "0.");
	if (isFlag)
	{
		if (lstrlenA(szLogid) >= 16)
			szLogid[16] = 0;
	}
	else
	{
		if (lstrlenA(szLogid) >= 16)
			szLogid[17] = 0;
	}
	strcat_s(szResult, szLogid);
	strResult = szResult;
	strResult = aip::base64_encode(strResult.c_str(), strResult.length());
	return strResult;
}

bool CBaiduParse::GetloginBassInfo(BaiduUserInfo& baseInfo, const std::string strCookie)
{
	HttpRequest BaiduHttp;
	BaiduHttp.SetRequestCookies(strCookie);
	BaiduHttp.Send(GET, HOME_URL);
	std::string strResult(BaiduHttp.GetResponseText());
//	WriteFileBuffer(".\\1.txt", (char*)strResult.c_str(), strResult.length());
//	strResult = GetTextMid(strResult, "context=", ";\n            var yunData");
	REGEXVALUE rv = GetRegexValue(strResult, "context=(.*?);\\s");
	if (rv.size() < 1)
		return false;
	strResult = rv.at(0);
	//WriteFileBuffer(".\\2.txt", (char*)strResult.c_str(), strResult.length());
	rapidjson::Document dc;
	dc.Parse(strResult.c_str());
	if (!dc.IsObject())
		return false;
	if (dc.HasMember("bdstoken") && dc["bdstoken"].IsString())
		baseInfo.bdstoken = dc["bdstoken"].GetString();
	if (dc.HasMember("username") && dc["username"].IsString())
		baseInfo.strUserName = dc["username"].GetString();
	if (dc.HasMember("photo") && dc["photo"].IsString())
		baseInfo.strHeadImageUrl = dc["photo"].GetString();
	if (dc.HasMember("is_vip") && dc["is_vip"].IsInt())
		baseInfo.is_vip = dc["is_vip"].GetInt();
	return true;
}

REGEXVALUE CBaiduParse::GetRegexValue(const std::string strvalue, const std::string strRegx)
{
	REGEXVALUE rvResult;
	if (strvalue.empty() || strRegx.empty())
		return rvResult;
	boost::regex expr(strRegx);
	boost::smatch what;
	if (boost::regex_search(strvalue, what, expr))
	{
		size_t length = what.size();
		for (size_t i = 1; i < length;++i)
		{
			if (what[i].matched)
			{
			/*	std::vector<char>val;
				val.resize(what[i].str().length());
				memcpy(val.data(), &what[i].str()[1], what[i].str().length() - 2);*/
				rvResult.push_back(what[i].str());
			}
		}
	}
	return rvResult;
}

void CBaiduParse::ShowInputVerCodeDlg()
{
	::DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_VERCODE), GetDesktopWindow(), ImageProc);
}

std::string CBaiduParse::GetBaiduFileListInfo(const std::string& path, const std::string strCookie)
{
	std::string strResultJson;
	FileTypeArray fileInfoResult;
	std::string strFileUrl,strResult;
	ZeroMemory(&fileInfoResult, sizeof(fileInfoResult));
	if (strCookie.empty() || path.empty())
		return strResultJson;
	BaiduUserInfo userinfo;
	if (!GetloginBassInfo(userinfo, strCookie))
		return strResultJson;
	strFileUrl = str(boost::format(FILE_LIST_URL) % URL_Coding(path.c_str()).c_str() % userinfo.bdstoken % GetLogid() % std::to_string(getTimeStamp()));
	HttpRequest BaiduHttp;
	BaiduHttp.SetRequestCookies(strCookie);
	BaiduHttp.Send(GET, strFileUrl);
	strResult = BaiduHttp.GetResponseText();
	rapidjson::Document dc;
	dc.Parse(strResult.c_str());
	if (!dc.IsObject())
		return strResultJson;
	if (dc.HasMember("list") && dc["list"].IsArray())
	{
		for (auto&v:dc["list"].GetArray())
		{
			if (v.IsObject())
			{
				BaiduFileInfo item;
				if (v.HasMember("category") && v["category"].IsInt())
					item.nCategory = v["category"].GetInt();
				if (v.HasMember("isdir") && v["isdir"].IsInt())
					item.nisdir = v["isdir"].GetInt();
				if (v.HasMember("path") && v["path"].IsString())
					item.strPath = v["path"].GetString();
				if (v.HasMember("server_filename") && v["server_filename"].IsString())
					item.server_filename = v["server_filename"].GetString();
				if (v.HasMember("size") && v["size"].IsUint())
					item.size = v["size"].GetUint();
				if (v.HasMember("server_ctime") && v["server_ctime"].IsUint())
					item.server_ctime = v["server_ctime"].GetUint();
				if (v.HasMember("fs_id") && v["fs_id"].IsUint64())
					item.fs_id = std::to_string(v["fs_id"].GetUint64());
				if (!item.nisdir)
					item.strFileType = ::PathFindExtensionA(Utf8_To_Gbk(item.server_filename.c_str()).c_str());
				fileInfoResult.push_back(item);
			}
		}
	}
	rapidjson::Document docjson;
	docjson.SetObject();
	rapidjson::Value UsrName(rapidjson::kStringType);
	UsrName.SetString(userinfo.strUserName.c_str(), userinfo.strUserName.length(), docjson.GetAllocator());
	docjson.AddMember(rapidjson::StringRef("UserName"), UsrName, docjson.GetAllocator());
	rapidjson::Value UsrHeaderImage(rapidjson::kStringType);
	UsrHeaderImage.SetString(userinfo.strHeadImageUrl.c_str(), userinfo.strHeadImageUrl.length(), docjson.GetAllocator());
	docjson.AddMember(rapidjson::StringRef("UserHeader"), UsrHeaderImage, docjson.GetAllocator());
	rapidjson::Value array_list(rapidjson::kArrayType);
	for (size_t i = 0; i < fileInfoResult.size();i++)
	{
		rapidjson::Value itemObject(rapidjson::kObjectType);
		rapidjson::Value isdir(rapidjson::kNumberType);
		isdir.SetInt(fileInfoResult[i].nisdir);
		itemObject.AddMember(rapidjson::StringRef("isdir"), isdir, docjson.GetAllocator());
		itemObject.AddMember(rapidjson::StringRef("name"), rapidjson::StringRef(fileInfoResult[i].server_filename.c_str()), docjson.GetAllocator());
		std::string strFilesize = GetFileSizeType(fileInfoResult[i].size);
		rapidjson::Value FileSize(rapidjson::kStringType);
		FileSize.SetString(strFilesize.c_str(), strFilesize.length(), docjson.GetAllocator());
		itemObject.AddMember(rapidjson::StringRef("Size"), FileSize, docjson.GetAllocator());
		rapidjson::Value nCategory(rapidjson::kNumberType);
		nCategory.SetInt(fileInfoResult[i].nCategory);
		itemObject.AddMember(rapidjson::StringRef("nCategory"), nCategory, docjson.GetAllocator());
		itemObject.AddMember(rapidjson::StringRef("path"), rapidjson::StringRef(fileInfoResult[i].strPath.c_str()), docjson.GetAllocator());
		itemObject.AddMember(rapidjson::StringRef("FileType"), rapidjson::StringRef(fileInfoResult[i].strFileType.c_str()), docjson.GetAllocator());
		std::string strTime = timestampToDate(fileInfoResult[i].server_ctime);
		rapidjson::Value FileTime(rapidjson::kStringType);
		FileTime.SetString(strTime.c_str(), strTime.length(), docjson.GetAllocator());
		itemObject.AddMember(rapidjson::StringRef("ChangeTime"), FileTime, docjson.GetAllocator());
		rapidjson::Value fs_id(rapidjson::kStringType);
		fs_id.SetString(fileInfoResult[i].fs_id.c_str(), fileInfoResult[i].fs_id.length(), docjson.GetAllocator());
		itemObject.AddMember(rapidjson::StringRef("fs_id"), fs_id, docjson.GetAllocator());
		array_list.PushBack(itemObject, docjson.GetAllocator());
		
	}
	docjson.AddMember(rapidjson::StringRef("data"), array_list, docjson.GetAllocator());
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	docjson.Accept(writer);
	strResultJson = buffer.GetString();
	//WriteFileBuffer(".\\1.txt",(PVOID)strResultJson.c_str(), strResultJson.length());
	return strResultJson;
}

std::string CBaiduParse::URL_Coding(const char* szSource, bool isletter /*= true*/, bool isUtf8 /*= true*/)
{
	CHAR szTemp[20];
	CHAR* buffer = nullptr;
	std::string strBuffer;
	std::string result;
	std::string strTemp;
	if (isUtf8)
		strBuffer = Gbk_To_Utf8(szSource);
	else
		strBuffer = szSource;
	int lens = strBuffer.length();
	buffer = new CHAR[lens + 1];
	if (!buffer)return result;
	ZeroMemory(buffer, lens + 1);
	memcpy(buffer, strBuffer.c_str(), lens);
	for (int i = 0; i<lens; i++)
	{
		ZeroMemory(szTemp, 20);
		if (isletter)
		{
			BYTE cbyte = *(buffer + i);
			if (cbyte > 44 && cbyte < 58 && 47 != cbyte)		//0-9
			{
				sprintf_s(szTemp, "%c", cbyte);
				strTemp = szTemp;
				result += strTemp;
			}
			else if (cbyte > 'A' && cbyte <= 'Z')	//A-Z
			{
				sprintf_s(szTemp, "%c", cbyte);
				strTemp = szTemp;
				result += strTemp;
			}
			else if (cbyte > 'a' && cbyte <= 'z')	//a-z
			{
				sprintf_s(szTemp, "%c", cbyte);
				strTemp = szTemp;
				result += strTemp;
			}
			else
			{
				sprintf_s(szTemp, "%02X", *(PBYTE)(buffer + i));
				strTemp = "%";
				strTemp += szTemp;
				result += strTemp;
			}
		}
		else
		{
			sprintf_s(szTemp, "%02X", *(PBYTE)(buffer + i));
			strTemp = "%";
			strTemp += szTemp;
			result += strTemp;
		}
	}
	delete[] buffer;
	return result;
}

std::string CBaiduParse::UnEscape(const char* strSource)
{
	std::string strResult;
	int nDestStep = 0;
	int nLength = strlen(strSource);
	if (!nLength || nLength < 6) return strResult;
	char* pResult = new char[nLength + 1];
	wchar_t* pWbuufer = nullptr;
	if (!pResult)
	{
		pResult = NULL;
		return strResult;
	}
	ZeroMemory(pResult, nLength + 1);
	for (int nPos = 0; nPos < nLength; nPos++)
	{
		if (strSource[nPos] == '\\' && strSource[nPos + 1] == 'u')
		{
			char szTemp[5];
			char szSource[5];
			ZeroMemory(szTemp, 5);
			ZeroMemory(szSource, 5);
			CopyMemory(szSource, (char*)strSource + nPos + 2, 4);
			sscanf_s(szSource, "%04X", szTemp);
			CopyMemory(pResult + nDestStep, szTemp, 4);
			nDestStep += 2;
		}
	}
	nDestStep += 2;
	pWbuufer = new wchar_t[nDestStep];
	if (!pWbuufer)
	{
		delete[] pWbuufer;
		pWbuufer = nullptr;
		return strResult;
	}
	ZeroMemory(pWbuufer, nDestStep);
	CopyMemory(pWbuufer, pResult, nDestStep);
	delete[] pResult;
	pResult = nullptr;
	CHAR* MultPtr = nullptr;
	int MultLen = -1;
	MultLen = ::WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, pWbuufer, -1, NULL, NULL, NULL, NULL);
	MultPtr = new CHAR[MultLen + 1];
	if (MultPtr)
	{
		ZeroMemory(MultPtr, MultLen + 1);
		::WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, pWbuufer, -1, MultPtr, MultLen, NULL, NULL);
		strResult = MultPtr;
		delete[] MultPtr;
		MultPtr = nullptr;
	}
	delete[] pWbuufer;
	pWbuufer = nullptr;
	return strResult;
}

float CBaiduParse::roundEx(float Floatnum)
{
	return (int)(Floatnum * 100 + 0.5) / 100.0;
}

std::string CBaiduParse::GetFileSizeType(double dSize)
{
	std::string szFileSize;
	if (dSize <1024)
	{
		szFileSize = str(boost::format("%.2f B") %roundEx(dSize));
	}
	else if (dSize >1024 && dSize < 1024 * 1024 * 1024 && dSize <1024 * 1024)
	{
		szFileSize = str(boost::format("%.2f KB") %roundEx(dSize / 1024));
	}
	else if (dSize >1024 * 1024 && dSize <1024 * 1024 * 1024)
	{
		szFileSize = str(boost::format("%.2f MB") %roundEx(dSize / 1024 / 1024));
	}
	else if (dSize >1024 * 1024 * 1024)
	{
		szFileSize = str(boost::format("%.2f GB") %roundEx(dSize / 1024 / 1024 / 1024));
	}
	return szFileSize;
}

std::string CBaiduParse::timestampToDate(ULONGLONG ctime)
{
	struct tm time;
	time_t tick = (time_t)ctime;
	::localtime_s(&time, &tick);
	char sztime[100];
	ZeroMemory(sztime, 100);
	::strftime(sztime, 100, "%Y-%m-%d", &time);
	return std::string(sztime);
}

DATETIME CBaiduParse::GetDateTime(const std::string& strTime)
{
	DATETIME rDateTime;
	ZeroMemory(&rDateTime, sizeof(DATETIME));
	if (strTime.empty())
		return rDateTime;
	std::vector<std::string> resultVec;
	std::string split = strTime + "-";
	std::string pattern = "-";
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
	if (resultVec.size()==3)
	{
		rDateTime.nYear = atoi(resultVec.at(0).c_str());
		rDateTime.nMonth = atoi(resultVec.at(1).c_str());
		rDateTime.nDay = atoi(resultVec.at(2).c_str());
	}
	return rDateTime;
}

std::string CBaiduParse::GetBaiduLocalFileAddr(const std::string path, const std::string strCookie)
{
	std::string strResult;
	if (path.empty() || strCookie.empty())
		return strResult;
	HttpRequest BaiduHttp;
	BaiduHttp.SetRequestCookies(strCookie);
	BaiduHttp.SetRequestHeader("User-Agent", "netdisk;6.0.0.12;PC;PC-Windows;10.0.16299;WindowsBaiduYunGuanJia");
	BaiduHttp.SetRequestHeader("Accept", " image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, */*");
	BaiduHttp.SetRequestHeader("Host", " d.pcs.baidu.com");
	std::string strRequestUrl = str(boost::format(DOWN_LOCAL_FILE) % path.c_str());
	BaiduHttp.Send(GET, strRequestUrl);
	strResult = BaiduHttp.GetResponseText();
	rapidjson::Document dc;
	dc.Parse(strResult.c_str());
	if (!dc.IsObject())
		return std::string("");

	return strResult;
}

std::string CBaiduParse::ShareBaiduFile(SHAREFILEINFO shareFileinfo, const std::string strCookie)
{
	std::string strResult;
	std::string sharDat;
	if (strCookie.empty())
		return strResult;
	HttpRequest BaiduHttp;
	BaiduHttp.SetRequestCookies(strCookie);
	BaiduUserInfo userinfo;
	if (!GetloginBassInfo(userinfo, strCookie))
		return strResult;
	std::string strShareUrl;
	if (shareFileinfo.nschannel)
	{
		strShareUrl = str(boost::format(SHARE_FILE_URL_1) % userinfo.bdstoken % GetLogid());
		sharDat = str(boost::format("schannel=4&channel_list=%%5B%%5D&period=%1%&pwd=%2%&fid_list=%%5B%3%%%5D") % shareFileinfo.strperiod % shareFileinfo.strPwd % shareFileinfo.strpath_list);
	}
	else
	{
		strShareUrl = str(boost::format(SHARE_FILE_URL_2) % userinfo.bdstoken % GetLogid());
		sharDat = str(boost::format("schannel=0&channel_list=%%5B%%5D&period=%1%&path_list=%%5B%%22%2%%%22%%5D") % shareFileinfo.strperiod % URL_Coding(Utf8_To_Gbk(shareFileinfo.strpath_list.c_str()).c_str()));
	}
	BaiduHttp.Send(POST, strShareUrl, sharDat);
	strResult = BaiduHttp.GetResponseText();
	return strResult;
}

std::string CBaiduParse::DeleteBaiduFile(const std::string strJsonData, const std::string strCookie)
{
	std::string bResult;
	if (strJsonData.empty() || strCookie.empty())
		return bResult;
	rapidjson::Document dc;
	dc.Parse(strJsonData.c_str());
	if (!dc.IsObject())
		return bResult;
	if (!dc.HasMember("data") && !dc["data"].IsArray())
	{
		return bResult;
	}
	rapidjson::Value datajson = dc["data"].GetArray();
	std::string strSupDir;
	std::string strSendData="[";
	for (size_t i = 0; i < datajson.Size(); i++)
	{
		if (i < (datajson.Size()-1))
		{
			if (datajson[i].HasMember("path") && datajson[i]["path"].IsString())
			{
				std::string strTempFile(Utf8_To_Gbk(datajson[i]["path"].GetString()));
				strSendData += "\"" + strTempFile + "\",";
			}
		}
		else{
			std::string strTempFile(Utf8_To_Gbk(datajson[i]["path"].GetString()));
			int npos = strTempFile.rfind("/");
			if (npos != std::string::npos)
			{
				strSupDir = strTempFile.substr(0, npos);
				if (strSupDir.empty())
					strSupDir = "/";
			}
			strSendData += "\"" + strTempFile + "\"]";
		}
	}
	strSendData = URL_Coding(strSendData.c_str());
	std::string strDeleteUrl("https://pan.baidu.com/api/filemanager?opera=delete&async=1&channel=chunlei&web=1&app_id=250528&clienttype=0&bdstoken=");
	HttpRequest BaiduHttp;
	BaiduHttp.SetRequestCookies(strCookie);
	BaiduHttp.SetRequestHeader("Accept", "image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, */*");
	BaiduHttp.SetRequestHeader("Content-Type", "application/x-www-form-urlencoded");
	BaiduUserInfo userinfo;
	if (!GetloginBassInfo(userinfo, strCookie))
		return bResult;
	strDeleteUrl += userinfo.bdstoken;
	BaiduHttp.Send(POST, strDeleteUrl, "filelist="+strSendData);
	strDeleteUrl = BaiduHttp.GetResponseText();
	dc.Parse(strDeleteUrl.c_str());
	if (!dc.IsObject())
		return bResult;
	if (!dc.HasMember("errno") && !dc["errno"].IsInt())
		return bResult;
	int errorn = dc["errno"].GetInt();
	if (errorn == 0)
		bResult = GetBaiduFileListInfo(strSupDir, strCookie);
	return bResult;
}

CBaiduParse::CBaiduParse()
{

}

INT_PTR CALLBACK CBaiduParse::ImageProc(_In_ HWND hwndDlg, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
						  HWND imageHwnd = ::GetDlgItem(hwndDlg, IDC_STATIC_IMG);
						  HttpRequest baiduHttp;
						  baiduHttp.Send(GET, m_vcCodeUrl);
						  responseData imageData = baiduHttp.GetResponseBody();
						  CImage img;
						  HBITMAP bithandel = NULL, hMole = NULL;
						  if (LodcomImage(imageData.data(), imageData.size(), img))
						  {
							  hMole = img.Detach();
							  bithandel = (HBITMAP)::SendMessage(imageHwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hMole);
							  DeleteObject(bithandel);
						  }

	}
		break;
	case WM_COMMAND:
	{
					   switch (LOWORD(wParam))
					   {
					   case IDC_BTN_OK:
					   {
										  char szCode[MAX_PATH];
										  ZeroMemory(szCode, MAX_PATH);
										  ::GetDlgItemTextA(hwndDlg, IDC_EDIT_CODE, szCode, MAX_PATH);
										  m_VerCode = szCode;
										  ::EndDialog(hwndDlg, 0);
					   }
						   break;
					   default:
						   break;
					   }
	}
	default:
		break;
	}
	return 0;
}

BOOL CBaiduParse::LodcomImage(LPVOID PmemIm, ULONG dwLen, CImage& ImgObj)
{
	LPVOID	  m_ImageBuf = NULL;
	BOOL bRet = FALSE;
	HGLOBAL	 Hglobal = ::GlobalAlloc(GMEM_MOVEABLE, dwLen);
	if (!Hglobal)
	{
		return bRet;
	}
	m_ImageBuf = ::GlobalLock(Hglobal);			//锁定全局分配的内存然后复制
	memset(m_ImageBuf, 0, dwLen);
	memcpy(m_ImageBuf, PmemIm, dwLen);
	::GlobalUnlock(Hglobal);		//解除锁定
	IStream	 *Pstream = NULL;
	HRESULT hr = ::CreateStreamOnHGlobal(Hglobal, TRUE, &Pstream);
	if (FAILED(hr))return bRet;
	hr = ImgObj.Load(Pstream);
	if (FAILED(hr))return bRet;
	Pstream->Release();
	bRet = TRUE;
	::GlobalFree(Hglobal);		//释放全局内存申请
	return bRet;
}

CBaiduParse::~CBaiduParse()
{

}

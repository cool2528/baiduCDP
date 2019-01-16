#include "WkeWindow.h"
#include <shlwapi.h>
#include <WinInet.h>
#include <fstream>
#include <stdexcept>
#include <boost/format.hpp>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#pragma comment(lib,"Wininet.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma warning(disable:4996)
std::mutex g_mutx;
WNDPROC CWkeWindow::m_oldProc = NULL;

void* CWkeWindow::AlloclocalHeap(const std::string& strBuffer)
{ 
	char* pResutPtr = nullptr;
	if (strBuffer.empty())
		return pResutPtr;
	int nLen = strBuffer.length() + 1;
	pResutPtr = new char[nLen];
	if (!pResutPtr)
		return pResutPtr;
	ZeroMemory(pResutPtr, nLen);
	CopyMemory(pResutPtr, strBuffer.c_str(),strBuffer.length());
	return pResutPtr;
}

std::shared_ptr<CWkeWindow> CWkeWindow::Instance = nullptr;
#if 1
void CWkeWindow::ParseAria2JsonInfo(const std::string& strJSon)
{
	rapidjson::Document dc;
	dc.Parse(strJSon.c_str());
	if (!dc.IsObject())
		return;
	std::string strDownStatus;
	std::string strGidItem;
	if (dc.HasMember("method") && dc["method"].IsString())
	{
		strDownStatus = dc["method"].GetString();
		if (dc.HasMember("params") && dc["params"].IsArray())
		{
			for (auto &v : dc["params"].GetArray())
			{
				if (!v.IsObject())
					continue;
				if (v.HasMember("gid") && v["gid"].IsString())
					strGidItem = v["gid"].GetString();
			}
		}
		if (strDownStatus == ARIA2_DOWNLOAD_COMPLATE)
		{
			PVOID sendPtr = AlloclocalHeap(strGidItem);
			::PostMessage(m_hwnd, ARIA2_DOWNLOAD_COMPLATE_MSG, NULL, (LPARAM)sendPtr);
		}
		else if (strDownStatus == ARIA2_DOWNLOAD_START)
		{
			PVOID sendPtr = AlloclocalHeap(strGidItem);
			::PostMessage(m_hwnd, ARIA2_DOWNLOAD_START_MSG, NULL, (LPARAM)sendPtr);
		}
		else if (strDownStatus == ARIA2_DOWNLOAD_STOP)
		{
			PVOID sendPtr = AlloclocalHeap(strGidItem);
			::PostMessage(m_hwnd, ARIA2_DOWNLOAD_STOP_MSG, NULL, (LPARAM)sendPtr);
		}
		else if (strDownStatus == ARIA2_DOWNLOAD_PAUSE)
		{
			PVOID sendPtr = AlloclocalHeap(strGidItem);
			::PostMessage(m_hwnd, ARIA2_DOWNLOAD_PAUSE_MSG, NULL, (LPARAM)sendPtr);
		}
		else if (strDownStatus == ARIA2_DOWNLOAD_ERROR)
		{
			PVOID sendPtr = AlloclocalHeap(strGidItem);
			::PostMessage(m_hwnd, ARIA2_DOWNLOAD_ERROR_MSG, NULL, (LPARAM)sendPtr);
		}
	}
	if (dc.HasMember("result") && dc["result"].IsArray())
	{
		if (dc["result"].GetArray().Size()==0)
		{
			return;
		}
		//开始组装json发送给UI线程处理渲染到界面
		rapidjson::Document sendJson;
		sendJson.SetObject();
		rapidjson::Value arrlist(rapidjson::kArrayType);
		int nErrorcount = 0;
		for (auto& v : dc["result"].GetArray())
		{
			rapidjson::Value sendItemObj(rapidjson::kObjectType);
			DOWNFILELISTINFO ItemValue;
			ZeroMemory(&ItemValue, sizeof(DOWNFILELISTINFO));
			ULONGLONG totalLength, completedLength, downloadSpeed;
			if (v.HasMember("totalLength") && v["totalLength"].IsString())
				sscanf_s(v["totalLength"].GetString(), "%I64d", &totalLength);
			if (v.HasMember("completedLength") && v["completedLength"].IsString())
				sscanf_s(v["completedLength"].GetString(), "%I64d", &completedLength);
			if (v.HasMember("downloadSpeed") && v["downloadSpeed"].IsString())
				sscanf_s(v["downloadSpeed"].GetString(), "%I64d", &downloadSpeed);
			if (v.HasMember("connections") && v["connections"].IsString())
				ItemValue.connections = v["connections"].GetString();
			if (v.HasMember("gid") && v["gid"].IsString())
				ItemValue.strFileGid = v["gid"].GetString();
			if (v.HasMember("status") && v["status"].IsString())
				ItemValue.strStatus = v["status"].GetString();
			if (v.HasMember("errorMessage") && v["errorMessage"].IsString() && v.HasMember("errorCode") && v["errorCode"].IsString())
			{
				nErrorcount++;
				ItemValue.nErrCode = atoi(v["errorCode"].GetString());
				ItemValue.strErrMessage = v["errorMessage"].GetString();
			}
			if (v.HasMember("files") && v["files"].IsArray())
			{
				for (auto& keyval : v["files"].GetArray())
				{
					if (keyval.HasMember("path") && keyval["path"].IsString())
					{
						char szName[MAX_PATH];
						ZeroMemory(szName, MAX_PATH);
						std::string szFileName = m_BaiduPare.Utf8_To_Gbk(keyval["path"].GetString());
						CopyMemory(szName, szFileName.c_str(), szFileName.length());
						PathStripPathA(szName);
						ItemValue.strFileName = szName;
					}


				}
			}
			ItemValue.Downloadpercentage = (size_t)Getpercentage(completedLength, totalLength);
			ItemValue.DownloadSpeed = GetFileSizeType(downloadSpeed);
			rapidjson::Value connections(rapidjson::kStringType);//当前服务器下载链接数量
			connections.SetString(ItemValue.connections.c_str(), ItemValue.connections.length(), sendJson.GetAllocator());
			sendItemObj.AddMember(rapidjson::StringRef("connections"), connections, sendJson.GetAllocator());
			rapidjson::Value downloadSpeedItem(rapidjson::kStringType);//下载速度每秒
			downloadSpeedItem.SetString(ItemValue.DownloadSpeed.c_str(), ItemValue.DownloadSpeed.length(), sendJson.GetAllocator());
			sendItemObj.AddMember(rapidjson::StringRef("downloadSpeed"), downloadSpeedItem, sendJson.GetAllocator());
			rapidjson::Value FileName(rapidjson::kStringType);//当前任务的文件名
			FileName.SetString(ItemValue.strFileName.c_str(), ItemValue.strFileName.length(), sendJson.GetAllocator());
			sendItemObj.AddMember(rapidjson::StringRef("name"), FileName, sendJson.GetAllocator());
			rapidjson::Value Gid(rapidjson::kStringType);//当前任务GID唯一标识符
			Gid.SetString(ItemValue.strFileGid.c_str(), ItemValue.strFileGid.length(), sendJson.GetAllocator());
			sendItemObj.AddMember(rapidjson::StringRef("gid"), Gid, sendJson.GetAllocator());
			rapidjson::Value Downloadpercentage(rapidjson::kNumberType);//当前下载的百分百
			Downloadpercentage.SetUint(ItemValue.Downloadpercentage);
			sendItemObj.AddMember(rapidjson::StringRef("progress"), Downloadpercentage, sendJson.GetAllocator());
			rapidjson::Value nErrCode(rapidjson::kNumberType);//下载错误代码
			nErrCode.SetUint(ItemValue.nErrCode);
			sendItemObj.AddMember(rapidjson::StringRef("errorCode"), nErrCode, sendJson.GetAllocator());
			rapidjson::Value ErrMessage(rapidjson::kStringType);//下载错误原因
			ErrMessage.SetString(ItemValue.strErrMessage.c_str(), ItemValue.strErrMessage.length(), sendJson.GetAllocator());
			sendItemObj.AddMember(rapidjson::StringRef("errorMessage"), ErrMessage, sendJson.GetAllocator());
			rapidjson::Value status(rapidjson::kStringType);//下载文件状态
			status.SetString(ItemValue.strStatus.c_str(), ItemValue.strStatus.length(), sendJson.GetAllocator());
			sendItemObj.AddMember(rapidjson::StringRef("status"), status, sendJson.GetAllocator());
			arrlist.PushBack(sendItemObj, sendJson.GetAllocator());
		}
		//发送给UI线程
		sendJson.AddMember(rapidjson::StringRef("data"), arrlist, sendJson.GetAllocator());
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		sendJson.Accept(writer);
		std::string strResultJson = buffer.GetString();
		strResultJson = m_BaiduPare.Gbk_To_Utf8(strResultJson.c_str());
		PVOID pSendDataPtr = AlloclocalHeap(strResultJson);
		if (!nErrorcount)
		{
			::PostMessage(m_hwnd, ARIA2_UPDATE_TELLACTIVE_LIST_MSG, NULL, (LPARAM)pSendDataPtr);
		}


	}
	if (dc.HasMember("result") && dc["result"].IsObject())
	{
		rapidjson::Value itemObj = dc["result"].GetObjectW();
		if (itemObj.HasMember("numActive") && itemObj["numActive"].IsString())
			numActive = atol(itemObj["numActive"].GetString());
		if (itemObj.HasMember("numStopped") && itemObj["numStopped"].IsString())
			numStopped = atol(itemObj["numStopped"].GetString());
		if (itemObj.HasMember("numWaiting") && itemObj["numWaiting"].IsString())
			numWaiting = atol(itemObj["numWaiting"].GetString());
		if (itemObj.HasMember("errorCode") && itemObj["errorCode"].IsString())
		{
			//开始组装json发送给UI线程处理渲染到界面
			rapidjson::Document sendJson;
			std::string strRetryUrl;
			sendJson.SetObject();
			rapidjson::Value arrlist(rapidjson::kArrayType);
			rapidjson::Value sendItemObj(rapidjson::kObjectType);
			DOWNFILELISTINFO ItemValue;
			ZeroMemory(&ItemValue, sizeof(DOWNFILELISTINFO));
			ULONGLONG totalLength, completedLength, downloadSpeed;
			if (itemObj.HasMember("totalLength") && itemObj["totalLength"].IsString())
				sscanf_s(itemObj["totalLength"].GetString(), "%I64d", &totalLength);
			if (itemObj.HasMember("completedLength") && itemObj["completedLength"].IsString())
				sscanf_s(itemObj["completedLength"].GetString(), "%I64d", &completedLength);
			if (itemObj.HasMember("downloadSpeed") && itemObj["downloadSpeed"].IsString())
				sscanf_s(itemObj["downloadSpeed"].GetString(), "%I64d", &downloadSpeed);
			if (itemObj.HasMember("connections") && itemObj["connections"].IsString())
				ItemValue.connections = itemObj["connections"].GetString();
			if (itemObj.HasMember("gid") && itemObj["gid"].IsString())
				ItemValue.strFileGid = itemObj["gid"].GetString();

				ItemValue.nErrCode = atoi(itemObj["errorCode"].GetString());
				ItemValue.strErrMessage = itemObj["errorMessage"].GetString();

			if (itemObj.HasMember("files") && itemObj["files"].IsArray())
			{
				for (auto& keyval : itemObj["files"].GetArray())
				{
					if (keyval.HasMember("path") && keyval["path"].IsString())
					{
						char szName[MAX_PATH];
						ZeroMemory(szName, MAX_PATH);
						std::string szFileName = m_BaiduPare.Utf8_To_Gbk(keyval["path"].GetString());
						CopyMemory(szName, szFileName.c_str(), szFileName.length());
						PathStripPathA(szName);
						ItemValue.strFileName = szName;
					}
					if (keyval.HasMember("uris") && keyval["uris"].IsArray())
					{
						rapidjson::Value urisArr = keyval["uris"].GetArray();
						if (urisArr.Size())
						{
							if (urisArr[0].HasMember("uri") && urisArr[0]["uri"].IsString())
							{
								strRetryUrl = urisArr[0]["uri"].GetString();
							}
						}
					}

				}
			}
			IsRetryExist(ItemValue.strFileName);
			if (!IsRetryCountFinish(ItemValue.strFileName))
			{
				//清除下载失败的文件内存缓存
				std::string strformat = str(boost::format(ARIA2_PURGEDOWNLOAD_RESULT) % ItemValue.strFileGid);
				PVOID AllocPtr = AlloclocalHeap(strformat);
				PostMessage(m_hwnd, ARIA2_PURGEDOWNLOAD_MSG, NULL, (LPARAM)AllocPtr);
				REQUESTINFO rResult;
				std::string strJson;
				ZeroMemory(&rResult, sizeof(REQUESTINFO));
				GetInstance()->GetShareFileItem(ItemValue.strFileName,rResult);
				if (!rResult.strFileName.empty() && !rResult.strDownloadUrl.empty())
				{
					HttpRequest BaiduHttp;
					BaiduHttp.SetRequestCookies(rResult.strCookies);
					BaiduHttp.Send(HEAD, rResult.strDownloadUrl);
					rResult.strDownloadUrl = GetInstance()->m_BaiduPare.GetTextMid(BaiduHttp.GetallResponseHeaders(), "Location: ", "\r\n");
					rResult.strSavePath = "d:/pdf";
					strJson = GetInstance()->CreateDowndAria2Json(rResult);
					//GetInstance()->SendText(strJson);
					PVOID addurlPtr = AlloclocalHeap(strJson);
					PostMessage(m_hwnd, ARIA2_RETRYADDURL_MSG, NULL, (LPARAM)addurlPtr);
					//重试一次后增加一次重试的次数
					addRetryCount(ItemValue.strFileName);
				}
			}
			else
			{
				ItemValue.Downloadpercentage = (size_t)Getpercentage(completedLength, totalLength);
				ItemValue.DownloadSpeed = GetFileSizeType(downloadSpeed);
				rapidjson::Value connections(rapidjson::kStringType);//当前服务器下载链接数量
				connections.SetString(ItemValue.connections.c_str(), ItemValue.connections.length(), sendJson.GetAllocator());
				sendItemObj.AddMember(rapidjson::StringRef("connections"), connections, sendJson.GetAllocator());
				rapidjson::Value downloadSpeedItem(rapidjson::kStringType);//下载速度每秒
				downloadSpeedItem.SetString(ItemValue.DownloadSpeed.c_str(), ItemValue.DownloadSpeed.length(), sendJson.GetAllocator());
				sendItemObj.AddMember(rapidjson::StringRef("downloadSpeed"), downloadSpeedItem, sendJson.GetAllocator());
				rapidjson::Value FileName(rapidjson::kStringType);//当前任务的文件名
				FileName.SetString(ItemValue.strFileName.c_str(), ItemValue.strFileName.length(), sendJson.GetAllocator());
				sendItemObj.AddMember(rapidjson::StringRef("name"), FileName, sendJson.GetAllocator());
				rapidjson::Value Gid(rapidjson::kStringType);//当前任务GID唯一标识符
				Gid.SetString(ItemValue.strFileGid.c_str(), ItemValue.strFileGid.length(), sendJson.GetAllocator());
				sendItemObj.AddMember(rapidjson::StringRef("gid"), Gid, sendJson.GetAllocator());
				rapidjson::Value Downloadpercentage(rapidjson::kNumberType);//当前下载的百分百
				Downloadpercentage.SetUint(ItemValue.Downloadpercentage);
				sendItemObj.AddMember(rapidjson::StringRef("progress"), Downloadpercentage, sendJson.GetAllocator());
				rapidjson::Value nErrCode(rapidjson::kNumberType);//下载错误代码
				nErrCode.SetUint(ItemValue.nErrCode);
				sendItemObj.AddMember(rapidjson::StringRef("errorCode"), nErrCode, sendJson.GetAllocator());
				rapidjson::Value ErrMessage(rapidjson::kStringType);//下载错误原因
				ErrMessage.SetString(ItemValue.strErrMessage.c_str(), ItemValue.strErrMessage.length(), sendJson.GetAllocator());
				sendItemObj.AddMember(rapidjson::StringRef("errorMessage"), ErrMessage, sendJson.GetAllocator());
				arrlist.PushBack(sendItemObj, sendJson.GetAllocator());
				//发送给UI线程
				sendJson.AddMember(rapidjson::StringRef("data"), arrlist, sendJson.GetAllocator());
				rapidjson::StringBuffer buffer;
				rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
				sendJson.Accept(writer);
				std::string strResultJson = buffer.GetString();
				strResultJson = m_BaiduPare.Gbk_To_Utf8(strResultJson.c_str());
				PVOID pSendDataPtr = AlloclocalHeap(strResultJson);
				::PostMessage(m_hwnd, ARIA2_UPDATE_TELLERROR_LIST_MSG, NULL, (LPARAM)pSendDataPtr);
			}
		}
	}
}
#endif
std::string CWkeWindow::GetFileSizeType(double dSize)
{
	std::string szFileSize;
	if (dSize <1024)
	{
		szFileSize = str(boost::format("%.2f B") % m_BaiduPare.roundEx(dSize));
	}
	else if (dSize >1024 && dSize < 1024 * 1024 * 1024 && dSize <1024 * 1024)
	{
		szFileSize = str(boost::format("%.2f KB") % m_BaiduPare.roundEx(dSize / 1024));
	}
	else if (dSize >1024 * 1024 && dSize <1024 * 1024 * 1024)
	{
		szFileSize = str(boost::format("%.2f MB") % m_BaiduPare.roundEx(dSize / 1024 / 1024));
	}
	else if (dSize >1024 * 1024 * 1024)
	{
		szFileSize = str(boost::format("%.2f GB") % m_BaiduPare.roundEx(dSize / 1024 / 1024 / 1024));
	}
	return szFileSize;
}

double CWkeWindow::Getpercentage(double completedLength, double totalLength)
{
	return completedLength / totalLength * 100;
}


LRESULT CALLBACK CWkeWindow::MainProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	static int index  = 0;
	switch (uMsg)
	{
	case ARIA2_UPDATE_TELLACTIVE_LIST_MSG:	//更新当前正在下载的任务列表
	{
				if (lParam)
				{
					const char* pbuffer = (const char*)lParam;
					std::string strJsonData(pbuffer);
					delete pbuffer;
					GetInstance()->UpdateDownloadList(strJsonData);
				}
	}
	break;
	case ARIA2_PURGEDOWNLOAD_MSG: //清除下载错误的内存缓存
	{
		if (lParam)
		{
			const char* pbuffer = (const char*)lParam;
			std::string strJsonData(pbuffer);
			LOG(INFO) << "清除下载错误内存缓存" << strJsonData.c_str();
			delete pbuffer;
			GetInstance()->SendText(GetInstance()->m_BaiduPare.Gbk_To_Utf8(strJsonData.c_str()));
		}
	}
	break;
	case ARIA2_RETRYADDURL_MSG: //从新下载添加
	{
		if (lParam)
		{
			const char* pbuffer = (const char*)lParam;
			std::string strJsonData(pbuffer);
			delete pbuffer;
			LOG(INFO) << "重新下载添加" << strJsonData.c_str();
			GetInstance()->SendText(strJsonData);
		}
	}
	break;
	case ARIA2_UPDATE_TELLERROR_LIST_MSG://更新下载失败的文件任务列表
	{
		if (lParam)
		{
			const char* pbuffer = (const char*)lParam;
			std::string strJsonData(pbuffer);
			delete pbuffer;
			static auto updateErrorList = [](const std::string strJosn) {
				jsExecState es = wkeGlobalExec(app.window);
				jsValue thisObject = jsGetGlobal(es, "app");
				jsValue func = jsGet(es, thisObject, "updatedownloadErrorList");
				jsValue jsArg[1] = { jsString(es, strJosn.c_str()) };
				jsCall(es, func, thisObject, jsArg, 1);
			};
			LOG(INFO) << "更新下载失败的文件任务列表" << strJsonData.c_str();
			updateErrorList(strJsonData);
		}

	}
	break;
	case ARIA2_DOWNLOAD_COMPLATE_MSG: //某个任务下载完成
	{
		if (lParam)
		{
			const char* pbuffer = (const char*)lParam;
			std::string strGID(pbuffer);
			delete pbuffer;
			std::string strFileName = GetInstance()->GetFileCompletedInfo(strGID);
			if (strFileName.empty())
			{
				HttpRequest aria2_http;
				std::string strformat = str(boost::format(ARIA2_TELLSTATUS_SENDDATA) % strGID);
				aria2_http.Send(POST, ARIA2_HTTP_REQUESTURL, strformat);
				std::string strResult = aria2_http.GetResponseText();
				strFileName = GetInstance()->GetTellStatusFileName(strResult);
				strFileName = GetInstance()->m_BaiduPare.Gbk_To_Utf8(strFileName.c_str());
			}
			//更新数据到已下载完成列表
			static auto addSussedList = [](const std::string Gid) {
				std::string buffer = str(boost::format("{\"name\":\"%1%\",\"ChangeTime\":\"%2%\"}") % Gid % GetInstance()->m_BaiduPare.timestampToDate((ULONGLONG)time(NULL)));
				jsExecState es = wkeGlobalExec(app.window);
				jsValue thisObject = jsGetGlobal(es, "app");
				jsValue func = jsGet(es, thisObject, "updatedownloadSussedList");
				jsValue jsArg[1] = { jsString(es, buffer.c_str()) };
				jsCall(es, func, thisObject, jsArg, 1);
			};
			LOG(INFO) << "某个任务下载完成" << strFileName.c_str();
			addSussedList(strFileName);
		}
	}
	break;
	case ARIA2_DOWNLOAD_START_MSG: //某个任务开始下载
	{
		if (lParam)
		{
			const char* pbuffer = (const char*)lParam;
			std::string strGID(pbuffer);
			delete pbuffer;
			LOG(INFO) << "某个任务开始下载" << strGID.c_str();

		}
	}
	break;
	case ARIA2_DOWNLOAD_STOP_MSG: //某个任务停止下载
	{
		LOG(INFO) << "某个任务停止下载";
	}
	break;
	case ARIA2_DOWNLOAD_PAUSE_MSG: //某个任务 暂停下载
	{
		LOG(INFO) << "某个任务 暂停下载";
	}
	break;
	case ARIA2_DOWNLOAD_ERROR_MSG: //某个任务下载出错
	{
			if (lParam)
			{
				const char* pbuffer = (const char*)lParam;
				std::string strGID(pbuffer);
				delete pbuffer;
				std::string strformat = str(boost::format(ARIA2_TELLSTATUS_SENDDATA) % strGID);
				LOG(INFO) << "某个任务下载出错 这里会重新添加下载" << strformat.c_str();
				GetInstance()->SendText(GetInstance()->m_BaiduPare.Gbk_To_Utf8(strformat.c_str()));
			}
	}
	break;
	default:
		break;
	}
	return CallWindowProc(m_oldProc, hwnd, uMsg, wParam, lParam);
}

void CWkeWindow::AddShareFileItem(REQUESTINFO item)
{
	bool bResult = false;
	for (size_t i =0;i<m_ShareFileArray.size();i++)
	{
		auto & value = m_ShareFileArray.at(i);
		if (value.strFileName == item.strFileName)
		{
			bResult = true;
			break;
		}
	}
	if (!bResult)
		m_ShareFileArray.push_back(item);
}

void CWkeWindow::GetShareFileItem(const std::string& strFileName,REQUESTINFO& Result)
{

	for (size_t i=0;i<m_ShareFileArray.size();i++)
	{
		auto & value = m_ShareFileArray.at(i);
		if (value.strFileName == strFileName)
		{
			Result.strCookies = value.strCookies;
			Result.strDownloadUrl = value.strDownloadUrl;
			Result.strFileName = value.strFileName;
			Result.strSavePath = value.strSavePath;
			break;
		}
	}
}

bool CWkeWindow::IsRetryCountFinish(const std::string& strFileName)
{
	bool bResult = false;
	for (size_t i =0;i<m_RetryArray.size();i++)
	{
		auto & value = m_RetryArray.at(i);
		if (value.strFileName == strFileName && value.nCount > 10)
		{
			bResult = true;
			break;
		}
	}
	return bResult;
}

bool CWkeWindow::IsRetryExist(const std::string& strFileName)
{
	bool bResult = false;
	RETRYCOUNT item;
	ZeroMemory(&item, sizeof(RETRYCOUNT));
	for (size_t i = 0; i < m_RetryArray.size(); i++)
	{
		auto & value = m_RetryArray.at(i);
		if (value.strFileName == strFileName)
		{
			bResult = true;
			break;
		}
	}
	if (!bResult)
	{
		item.strFileName = strFileName;
		m_RetryArray.push_back(item);
	}
	return bResult;
}

void CWkeWindow::addRetryCount(const std::string& strFileName)
{
	for (size_t i = 0; i < m_RetryArray.size(); i++)
	{
		auto & value = m_RetryArray.at(i);
		if (value.strFileName == strFileName)
		{
			value.nCount++;
			break;
		}
	}
}

std::string CWkeWindow::GetFileCompletedInfo(const std::string& strGid)
{
	std::string StrJson="{\"data\":";
	std::string strResult;
	jsExecState es = wkeGlobalExec(app.window);
	jsValue thisObject = jsGetGlobal(es, "app");
	jsValue func = jsGet(es, thisObject, "GetBackupListString");
	jsValue rString = jsCall(es, func, thisObject, nullptr, 0);
	jsType Type = jsTypeOf(rString);
	StrJson += jsToTempString(es, rString);
	StrJson += "}";
	rapidjson::Document dc;
	dc.Parse(StrJson.c_str());
	if (!dc.IsObject())
		return strResult;
	if (dc.HasMember("data") && dc["data"].IsArray())
	{
		for (auto& v:dc["data"].GetArray())
		{
			if (v.HasMember("gid") && v["gid"].IsString())
			{
				if (strGid == v["gid"].GetString())
				{
					if (v.HasMember("name") && v["name"].IsString())
						strResult = v["name"].GetString();
				}
			}
		}
	}
	return strResult;
}

std::string CWkeWindow::GetTellStatusFileName(const std::string& strJSon)
{
	std::string strResult;
	rapidjson::Document dc;
	dc.Parse(strJSon.c_str());
	if (!dc.IsObject())
	{
		return strResult;
	}
	if (dc.HasMember("result") && dc["result"].IsObject())
	{
		rapidjson::Value itemObj = dc["result"].GetObjectW();


			if (itemObj.HasMember("files") && itemObj["files"].IsArray())
			{
				for (auto& keyval : itemObj["files"].GetArray())
				{
					if (keyval.HasMember("path") && keyval["path"].IsString())
					{
						char szName[MAX_PATH];
						ZeroMemory(szName, MAX_PATH);
						std::string szFileName = m_BaiduPare.Utf8_To_Gbk(keyval["path"].GetString());
						CopyMemory(szName, szFileName.c_str(), szFileName.length());
						PathStripPathA(szName);
						strResult = szName;
					}
				}
			}
		}
	return strResult;
}

#if 1
void CWkeWindow::on_socket_init(websocketpp::connection_hdl hdl)
{

}

void CWkeWindow::on_message(websocketpp::connection_hdl hdl, message_ptr msg)
{
	DWORD dwThread = ::GetThreadId(::GetCurrentThread());
	websocketpp::frame::opcode::value opcodes = msg->get_opcode();
	if (opcodes == websocketpp::frame::opcode::text)
	{

		std::string strMsgText = msg->get_payload();
		ParseAria2JsonInfo(strMsgText);
	}
}

void CWkeWindow::on_open(websocketpp::connection_hdl hdl)
{
	m_hdl = hdl;
}

void CWkeWindow::on_close(websocketpp::connection_hdl hdl)
{
	hdl.reset();
}

void CWkeWindow::on_fail(websocketpp::connection_hdl hdl)
{
	hdl.reset();
}

void CWkeWindow::start(std::string uri)
{
	websocketpp::lib::error_code ec;
	client::connection_ptr con = m_WssClient.get_connection(uri, ec);
	if (ec) {
		LOG(ERROR) << "启动wss环境失败";
		return;
	}
	m_WssClient.connect(con);
	m_WssClient.run();
}
void CWkeWindow::Connect()
{
	try{
		start("ws://127.0.0.1:6800/jsonrpc");
	}
	catch (websocketpp::exception const & e) {
		//std::cout << e.what() << std::endl;
		LOG(ERROR) << e.what();
	}
	catch (std::exception const & e) {
		//std::cout << e.what() << std::endl;
		LOG(ERROR) << e.what();
	}
	catch (...) {
		LOG(ERROR) <<"void CWkeWindow::Connect() 未知的异常";
	}
}

void CWkeWindow::SendText(std::string& strText)
{
	if (m_hdl.lock().get())
	{
		//m_BaiduPare.WriteFileBuffer(".\\1.txt",(char*)strText.c_str(), strText.length());
		m_WssClient.send(m_hdl, strText, websocketpp::frame::opcode::text);
	}
}
#endif
BOOL CWkeWindow::RunAria2()
{
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	si.dwFlags = STARTF_USESHOWWINDOW;  // 指定wShowWindow成员有效
	si.wShowWindow = true;          // 此成员设为TRUE的话则显示新建进程的主窗口，
	std::string strCommandlineArg = str(boost::format("aria2c.exe --check-certificate=false --disable-ipv6=true --enable-rpc=true --quiet=false --rpc-allow-origin-all=true --rpc-listen-all=true --rpc-listen-port=6800 --rpc-secret=CDP --stop-with-process=%1%")\
		% std::to_string(GetCurrentProcessId()));
	BOOL bRet = ::CreateProcessA(NULL,           // 不在此指定可执行文件的文件名
		const_cast<char*>(strCommandlineArg.c_str()),      // 命令行参数
		NULL,           // 默认进程安全性
		NULL,           // 默认线程安全性
		FALSE,          // 指定当前进程内的句柄不可以被子进程继承
		CREATE_NEW_CONSOLE, // 为新进程创建一个新的控制台窗口
		NULL,           // 使用本进程的环境变量
		NULL,           // 使用本进程的驱动器和目录
		&si,
		&pi);
	::CloseHandle(pi.hProcess);
	return bRet;
}
void CALLBACK CWkeWindow::TimeProc(HWND hwnd, UINT message, UINT iTimerID, DWORD dwTime)
{
#if 1
	GetInstance()->SendText(GetInstance()->m_BaiduPare.Gbk_To_Utf8(ARIA2_GETGLOBAL_STATUS));
	if (GetInstance()->numActive > 0 || GetInstance()->numWaiting > 0)
	{
		std::string strWaiting;
		GetInstance()->SendText(GetInstance()->m_BaiduPare.Gbk_To_Utf8(ARIA2_TELLACTICE_SENDDATA));
		strWaiting = str(boost::format(ARIA2_TELLSTOPPED) % GetInstance()->numStopped);
		GetInstance()->SendText(GetInstance()->m_BaiduPare.Gbk_To_Utf8(strWaiting.c_str()));
		strWaiting = str(boost::format(ARIA2_TELLWAITING) % GetInstance()->numWaiting);
		GetInstance()->SendText(GetInstance()->m_BaiduPare.Gbk_To_Utf8(strWaiting.c_str()));
		wkeRunJS(app.window, "app.backupdownloadListinfo = app.downloadListinfo;");
	}
	if (GetInstance()->numActive == 0 && GetInstance()->numWaiting == 0)
	{
		GetInstance()->UpdateDownloadList("{\"data\":[]}");
	}
#endif
}

void CWkeWindow::runApp(Application* app)
{
	DWORD dwThread = ::GetThreadId(::GetCurrentThread());
	memset(app, 0, sizeof(Application));
	app->url = L"http://baiducdp.com/ui/element.html"; // 使用hook的方式加载资源

	if (!createWebWindow(app)) {
		PostQuitMessage(0);
		return;
	}
	runMessageLoop(app);
}
std::string CWkeWindow::CarkUrl(const char* url)
{
	std::string result = "http://";
	URL_COMPONENTSA uc;
	char Scheme[1000];
	char HostName[1000];
	char UserName[1000];
	char Password[1000];
	char UrlPath[1000];
	char ExtraInfo[1000];

	uc.dwStructSize = sizeof(uc);
	uc.lpszScheme = Scheme;
	uc.lpszHostName = HostName;
	uc.lpszUserName = UserName;
	uc.lpszPassword = Password;
	uc.lpszUrlPath = UrlPath;
	uc.lpszExtraInfo = ExtraInfo;

	uc.dwSchemeLength = 1000;
	uc.dwHostNameLength = 1000;
	uc.dwUserNameLength = 1000;
	uc.dwPasswordLength = 1000;
	uc.dwUrlPathLength = 1000;
	uc.dwExtraInfoLength = 1000;
	InternetCrackUrlA(url, 0, 0, &uc);
	return (result += HostName);

}
bool CWkeWindow::InintMiniBlink()
{
	bool bResult = false;
	std::vector<wchar_t> mbPath;
	mbPath.resize(MAX_PATH);
	::GetModuleFileName(nullptr, mbPath.data(), MAX_PATH);
	if (!::PathRemoveFileSpec(mbPath.data())) return bResult;
	if (!PathAppendW(mbPath.data(), MBDLL_NAME))return bResult;
	if (!::PathFileExists(mbPath.data()))
	{
		::MessageBox(NULL, L"缺少miniblink 核心 node.dll文件!", L"错误", NULL);
		return bResult;
	}
	wkeSetWkeDllPath(mbPath.data());
	wkeInitialize();
	bResult = true;
	return bResult;
}
void CWkeWindow::quitApplication(Application* app)
{
	if (app->window) {
		wkeDestroyWebWindow(app->window);
		app->window = NULL;
	}
}
void CWkeWindow::runMessageLoop(Application* app)
{
	MSG msg = { 0 };
	while (GetMessageW(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}
bool CWkeWindow::createWebWindow(Application* app)
{
	app->window = wkeCreateWebWindow(WKE_WINDOW_TYPE_TRANSPARENT, NULL, 0, 0, 1024, 700);
	if (!app->window)
		return false;
	//设置全局cookies/缓存路径
	TCHAR szCookiePath[MAX_PATH];
	ZeroMemory(szCookiePath, MAX_PATH);
	::GetTempPath(MAX_PATH, szCookiePath);
	std::wstring szRootPath(szCookiePath);
	wcscat_s(szCookiePath, _T("cookie.tmp"));
	wkeSetCookieJarFullPath(app->window, szCookiePath);
	wkeSetLocalStorageFullPath(app->window, szRootPath.c_str());
	//保存主窗口的窗口句柄
 	m_hwnd = wkeGetWindowHandle(app->window);
 	m_oldProc =(WNDPROC)::SetWindowLongPtr(m_hwnd, GWL_WNDPROC, (DWORD)MainProc);
	::SetTimer(m_hwnd, UPDTAE_UI_TIMEID, 1000, TimeProc);
	//设置窗口的标题
	wkeSetWindowTitleW(app->window, APP_NAME);
	//开启Ajax 跨域请求支持
	wkeSetCspCheckEnable(app->window, false); 
	//当窗口被关闭时的回调函数可以让用户选择是否关闭
	wkeOnWindowClosing(app->window, OnCloseCallBack,this);
	//设置窗口销毁的回调函数
	wkeOnWindowDestroy(app->window, OnWindowDestroy, this);
	//当有新的webwindow窗口创建调用此回调
	wkeOnCreateView(app->window, onCreateView, this);
	//document文档加载成功回调函数
	wkeOnDocumentReady(app->window, OnDocumentReady, this);
	//设置网页请求前回调主要是为了加载本地资源
	wkeOnLoadUrlBegin(app->window, onLoadUrlBegin, this);
	//设置窗口居中显示
	wkeMoveToCenter(app->window);
	//设置请求的网址
	wkeLoadURLW(app->window, app->url.c_str());
	//设置全局js绑定函数
	wkeJsBindFunction("sysMenuCallBack", SysMenuJsNativeFunction, this, 1);
	//为webWindow 绑定一个判断百度是否登录成功的函数
	wkeJsBindFunction("isLoginBaidu", IsLoginBaidu, this, 0);
	// 绑定一个切换百度登录后切换目录的函数
	wkeJsBindFunction("SwitchDirPath", SwitchDirPath, this, 1);
	//绑定一个切换百度登录后下载网盘内文件的的函数
	wkeJsBindFunction("DownloadUserFile", DownloadUserFile, this, 1);
	//绑定一个百度登录后分享网盘文件的函数
	wkeJsBindFunction("ShareBaiduFile", ShareBaiduFile, this, 4);
	//绑定一个删除百度登录后网盘文件的函数
	wkeJsBindFunction("DeleteBaiduFile", DeleteBaiduFile, this, 1);
	//绑定一个下载分享链接文件的函数
	wkeJsBindFunction("DownShareFile", DownShareFile, this, 1);
	//AraiaPauseStartRemove
	//绑定一个操作aria2下载相关的函数
	wkeJsBindFunction("AraiaPauseStartRemove", AraiaPauseStartRemove, this, 2);
	//绑定一个退出百度登录账号的函数
	wkeJsBindFunction("LogOut", LogOut, this, 0);
	return true;
}

bool CWkeWindow::OnCloseCallBack(wkeWebView webWindow, void* param)
{
	if (param)
	{
		Application* app = (Application*)param;
		return IDYES == MessageBoxW(NULL, L"确定要退出程序吗？", L"wkexe", MB_YESNO | MB_ICONQUESTION);
	}
	return true;
}

void CWkeWindow::OnWindowDestroy(wkeWebView webWindow, void* param)
{
	Application* app = (Application*)param;
	app->window = NULL;
	PostQuitMessage(0);
}

void CWkeWindow::OnDocumentReady(wkeWebView webWindow, void* param)
{
	wkeShowWindow(webWindow, true);
}

bool CWkeWindow::onLoadUrlBegin(wkeWebView webView, void* param, const char* url, void *job)
{
	if (!param)return false;
	std::string localUrl = GetInstance()->CarkUrl(url);
	bool pos = localUrl == HOST_NAME ? true : false;
	if (pos) {
		const utf8* decodeURL = wkeUtilDecodeURLEscape(url);
		if (!decodeURL)
			return false;
		std::string urlString(decodeURL);
		std::string localPath = urlString.substr(sizeof(HOST_NAME)-1);

		std::wstring path = GetInstance()->getResourcesPath(GetInstance()->utf8ToUtf16(localPath));
		std::vector<char> buffer;

		GetInstance()->readJsFile(path.c_str(), &buffer);

		wkeNetSetData(job, buffer.data(), buffer.size());

		return true;
	}
	return false;
}

std::wstring CWkeWindow::getResourcesPath(const std::wstring name)
{
	std::wstring  bResult;
	std::wstring temp;
	std::vector<wchar_t> mbPath;
	mbPath.resize(MAX_PATH+1);
	::GetModuleFileName(nullptr, mbPath.data(), MAX_PATH);
	if (!::PathRemoveFileSpec(mbPath.data())) return bResult;
	temp += mbPath.data();
	temp += L"\\";
	bResult = temp + name;
	return bResult;
}

std::wstring CWkeWindow::utf8ToUtf16(const std::string& utf8String)
{
	std::wstring sResult;
	int nUTF8Len = MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), -1, NULL, NULL);
	wchar_t* pUTF8 = new wchar_t[nUTF8Len + 1];

	ZeroMemory(pUTF8, nUTF8Len + 1);
	MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), -1, pUTF8, nUTF8Len);
	sResult = pUTF8;
	delete[] pUTF8;

	return sResult;
}
DWORD CWkeWindow::ReadFileBuffer(std::string szFileName, PVOID* pFileBuffer)
{
	DWORD dwFileSize = NULL;
	DWORD dwAttribut = NULL;
	std::ifstream file;
	if (szFileName.empty())return dwFileSize;
	dwAttribut = ::GetFileAttributesA(szFileName.c_str());
	if (dwAttribut == INVALID_FILE_ATTRIBUTES || 0 != (dwAttribut & FILE_ATTRIBUTE_DIRECTORY))
	{
		dwFileSize = INVALID_FILE_ATTRIBUTES;
		return dwFileSize;
	}
	file.open(szFileName, std::ios::in | std::ios::binary);
	if (!file.is_open())
		return dwFileSize;
	file.seekg(0l, std::ios::end);
	dwFileSize = file.tellg();
	file.seekg(0l, std::ios::beg);
	*pFileBuffer = new BYTE[dwFileSize+1];
	if (!*pFileBuffer)
	{
		dwFileSize = NULL;
		file.close();
		return dwFileSize;
	}
	ZeroMemory(*pFileBuffer, dwFileSize+1);
	file.read((char*)*pFileBuffer, dwFileSize);
	file.close();
	return dwFileSize;
}
void CWkeWindow::readJsFile(const wchar_t* path, std::vector<char>* buffer)
{
	HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hFile) {
		//DebugBreak();
		return;
	}

	DWORD fileSizeHigh;
	const DWORD bufferSize = ::GetFileSize(hFile, &fileSizeHigh);

	DWORD numberOfBytesRead = 0;
	buffer->resize(bufferSize);
	BOOL b = ::ReadFile(hFile, &buffer->at(0), bufferSize, &numberOfBytesRead, nullptr);
	::CloseHandle(hFile);
}
// 回调:获取登录百度盘的cookie
void CWkeWindow::GetLoginCookieCallBack(wkeWebView webWindow, void* param)
{
	if (!param)return;
	wkeShowWindow(webWindow, true);
	std::string Url = wkeGetURL(webWindow);
	int npos = Url.find("disk/home?");
	if (npos != std::string::npos)
	{
		GetInstance()->isLogin = true;
		GetInstance()->strCookies = wkeGetCookie(webWindow);
		PVOID jsondata = nullptr;
		std::string strJsonData;
		char szModeleName[MAX_PATH];
		ZeroMemory(szModeleName, MAX_PATH);
		::GetModuleFileNameA(NULL, szModeleName, MAX_PATH);
		if (PathRemoveFileSpecA(szModeleName))
			strcat_s(szModeleName, "\\ui\\GlobalConfig.json");
		if (GetInstance()->ReadFileBuffer(szModeleName, &jsondata) && jsondata != nullptr)
		{
			strJsonData = (char*)jsondata;
			delete[] jsondata;
			rapidjson::Document dc;
			dc.Parse(strJsonData.c_str());
			if (!dc.IsObject())
				return;
			if (dc.HasMember("app") && dc["app"].IsObject())
			{
				rapidjson::Value app = dc["app"].GetObjectW();
				if (app.HasMember("UserInfo") && app["UserInfo"].IsObject())
				{
					rapidjson::Value UserInfo = app["UserInfo"].GetObjectW();
					if (UserInfo.HasMember("downloadSavePath") && UserInfo["downloadSavePath"].IsString())
					{
						GetInstance()->m_downloadSavePath = UserInfo["downloadSavePath"].GetString();
					}
				}
			}
			dc.SetObject();
			rapidjson::Value app(rapidjson::kObjectType);
			rapidjson::Value UserInfo(rapidjson::kObjectType);
			UserInfo.AddMember(rapidjson::StringRef("downloadSavePath"), rapidjson::StringRef(GetInstance()->m_downloadSavePath.c_str()), dc.GetAllocator());
			UserInfo.AddMember(rapidjson::StringRef("loginCookie"), rapidjson::StringRef(GetInstance()->strCookies.c_str()), dc.GetAllocator());
			app.AddMember(rapidjson::StringRef("UserInfo"), UserInfo, dc.GetAllocator());
			dc.AddMember(rapidjson::StringRef("app"), app, dc.GetAllocator());
			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
			dc.Accept(writer);
			strJsonData = buffer.GetString();
			GetInstance()->m_BaiduPare.WriteFileBuffer(szModeleName,const_cast<char*>(strJsonData.c_str()), strJsonData.length());
		}
		/*
		已经登录百度让登录百度的按钮隐藏
		*/
		wkeRunJS(app.window, "app.tablelistisShwo = true;");
		jsExecState es = wkeGlobalExec(app.window);
		jsValue thisObject = jsGetGlobal(es, "app");
		bool b = jsIsObject(thisObject);
		jsValue func = jsGet(es, thisObject, "updateBaiduList");
		std::string argJson = GetInstance()->m_BaiduPare.GetBaiduFileListInfo("/", GetInstance()->strCookies);
		//argJson = Utf8_To_Gbk(argJson.c_str());
		jsValue jsArg[1] = { jsString(es, argJson.c_str()) };
		jsCall(es, func, thisObject, jsArg, 1);
		wkeDestroyWebWindow(webWindow);
	}
}
wkeWebView CWkeWindow::onCreateView(wkeWebView webWindow, void* param, wkeNavigationType navType, const wkeString url, const wkeWindowFeatures* features)
{
	if (!param)return webWindow;
	std::string StrUrl(wkeGetString(url));
	::OutputDebugStringA(wkeGetString(url));
	wkeWebView newWindow = wkeCreateWebWindow(WKE_WINDOW_TYPE_POPUP, NULL, features->x, features->y, features->width, features->height);
	wkeSetCspCheckEnable(newWindow, false);
	if (StrUrl.find("pan.baidu.com") != std::string::npos)
	{
		wkeOnDocumentReady(newWindow, GetLoginCookieCallBack, param);
		static auto subCreateView = [](wkeWebView webWindow, void* param, wkeNavigationType navType, const wkeString url, const wkeWindowFeatures* features)->wkeWebView {
			RECT rc;
			::GetClientRect((HWND)param, &rc);
			wkeWebView newWindow = wkeCreateWebWindow(WKE_WINDOW_TYPE_CONTROL, (HWND)param, 0, 0, rc.right-rc.left, rc.bottom-rc.top);
			wkeShowWindow(newWindow, true);
			return newWindow;
		};
		wkeOnCreateView(newWindow, subCreateView, wkeGetWindowHandle(newWindow));
		//wkeSetDebugConfig(app.window, "showDevTools", GetInstance()->m_BaiduPare.Gbk_To_Utf8("E:\\Download\\miniblink-181214\\front_end\\inspector.html").c_str());
	}
	wkeShowWindow(newWindow, true);
	return newWindow;
}

void CWkeWindow::UpdateDownloadList(const std::string& strJson)
{
	jsExecState es = wkeGlobalExec(app.window);
	jsValue thisObject = jsGetGlobal(es, "app");
	jsValue func = jsGet(es, thisObject, "UpateDownloadlist");
	jsValue jsArg[1] = { jsString(es, strJson.c_str()) };
	jsCall(es, func, thisObject, jsArg, 1);
}

jsValue CWkeWindow::SysMenuJsNativeFunction(jsExecState es, void* param)
{
	if (!param)return jsUndefined();
	//获取参数的数量
	int argCount = jsArgCount(es);
	if (argCount < 1)
		return jsUndefined();
	jsType type = jsArgType(es, 0);
	if (JSTYPE_STRING != type)
		return jsUndefined();
	jsValue arg0 = jsArg(es, 0);
	std::string arg0str = jsToTempString(es, arg0);
	if (arg0str == CLOSE_MSG)
		GetInstance()->blinkClose();
	else if (arg0str == MAX_MSG)
		GetInstance()->blinkMaximize();
	else if (arg0str == MIN_MSG)
		GetInstance()->blinkMinimize();
	return jsUndefined();
}

jsValue CWkeWindow::LogOut(jsExecState es, void* param)
{
	if (!param)return jsUndefined();
	//获取参数的数量
	int argCount = jsArgCount(es);
	if (argCount !=0)
		return jsUndefined();
	//清除配置文件json里的cookies
	//删除临时目录下生成的cookie.tmp文件
	TCHAR szCookiePath[MAX_PATH];
	ZeroMemory(szCookiePath, MAX_PATH);
	::GetTempPath(MAX_PATH, szCookiePath);
	wcscat_s(szCookiePath, _T("cookie.tmp"));
	DeleteFile(szCookiePath);
	PVOID jsondata = nullptr;
	std::string strJsonData;
	char szModeleName[MAX_PATH];
	ZeroMemory(szModeleName, MAX_PATH);
	::GetModuleFileNameA(NULL, szModeleName, MAX_PATH);
	if (PathRemoveFileSpecA(szModeleName))
		strcat_s(szModeleName, "\\ui\\GlobalConfig.json");
	if (GetInstance()->ReadFileBuffer(szModeleName, &jsondata) && jsondata != nullptr)
	{
		strJsonData = (char*)jsondata;
		delete[] jsondata;
		rapidjson::Document dc;
		dc.Parse(strJsonData.c_str());
		if (!dc.IsObject())
			return jsUndefined();
		if (dc.HasMember("app") && dc["app"].IsObject())
		{
			rapidjson::Value app = dc["app"].GetObjectW();
			if (app.HasMember("UserInfo") && app["UserInfo"].IsObject())
			{
				rapidjson::Value UserInfo = app["UserInfo"].GetObjectW();
				if (UserInfo.HasMember("downloadSavePath") && UserInfo["downloadSavePath"].IsString())
				{
					GetInstance()->m_downloadSavePath = UserInfo["downloadSavePath"].GetString();
				}
			}
		}
		dc.SetObject();
		rapidjson::Value app(rapidjson::kObjectType);
		rapidjson::Value UserInfo(rapidjson::kObjectType);
		UserInfo.AddMember(rapidjson::StringRef("downloadSavePath"), rapidjson::StringRef(GetInstance()->m_downloadSavePath.c_str()), dc.GetAllocator());
		UserInfo.AddMember(rapidjson::StringRef("loginCookie"), rapidjson::StringRef(""), dc.GetAllocator());
		app.AddMember(rapidjson::StringRef("UserInfo"), UserInfo, dc.GetAllocator());
		dc.AddMember(rapidjson::StringRef("app"), app, dc.GetAllocator());
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		dc.Accept(writer);
		strJsonData = buffer.GetString();
		GetInstance()->m_BaiduPare.WriteFileBuffer(szModeleName, const_cast<char*>(strJsonData.c_str()), strJsonData.length());
	}
	wkeRunJS(app.window, "app.tablelistisShwo = false;");
	GetInstance()->strCookies = "";
	GetInstance()->isLogin = false;
	return jsUndefined();
}

void CWkeWindow::blinkMaximize()
{
	HWND hwnd = getHWND();
	static bool isMax = true;
	if (isMax)
		::PostMessageW(hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
	else
		::PostMessageW(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
	isMax = !isMax;
}

void CWkeWindow::blinkMinimize()
{
	HWND hwnd = getHWND();
	::PostMessageW(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
}

void CWkeWindow::blinkClose()
{
	HWND hwnd = getHWND();
	::PostMessageW(hwnd, WM_CLOSE, 0, 0);
}
HWND CWkeWindow::getHWND()
{

	if (app.window)
		return wkeGetWindowHandle(app.window);
	return NULL;
}

jsValue CWkeWindow::IsLoginBaidu(jsExecState es, void* param)
{
	if (!param)return jsUndefined();
	int argCount = jsArgCount(es);
	if (argCount != 0)
		return jsUndefined();
	jsValue result = jsInt(0);
	if (!GetInstance()->strCookies.empty() && GetInstance()->isLogin)
	{
		/*
		已经登录百度让登录百度的按钮隐藏
		*/
		wkeRunJS(app.window, "app.tablelistisShwo = true;");
		jsExecState es = wkeGlobalExec(app.window);
		jsValue thisObject = jsGetGlobal(es, "app");
		bool b = jsIsObject(thisObject);
		jsValue func = jsGet(es, thisObject, "updateBaiduList");
		std::string argJson = GetInstance()->m_BaiduPare.GetBaiduFileListInfo("/", GetInstance()->strCookies);
		//argJson = Utf8_To_Gbk(argJson.c_str());
		jsValue jsArg[1] = { jsString(es, argJson.c_str()) };
		jsCall(es, func, thisObject, jsArg, 1);
		result = jsInt(1);
	}
	PVOID jsondata = nullptr;
	std::string strJsonData;
	char szModeleName[MAX_PATH];
	ZeroMemory(szModeleName, MAX_PATH);
	::GetModuleFileNameA(NULL, szModeleName, MAX_PATH);
	if (PathRemoveFileSpecA(szModeleName))
		strcat_s(szModeleName, "\\ui\\GlobalConfig.json");
	if (GetInstance()->ReadFileBuffer(szModeleName, &jsondata) && jsondata != nullptr)
	{
		strJsonData = (char*)jsondata;
		delete[] jsondata;
		rapidjson::Document dc;
		dc.Parse(strJsonData.c_str());
		if(!dc.IsObject())
			return result;
		if (dc.HasMember("app") && dc["app"].IsObject())
		{
			rapidjson::Value app = dc["app"].GetObjectW();
			if (app.HasMember("UserInfo") && app["UserInfo"].IsObject())
			{
				rapidjson::Value UserInfo = app["UserInfo"].GetObjectW();
				if (UserInfo.HasMember("downloadSavePath") && UserInfo["downloadSavePath"].IsString())
				{
					GetInstance()->m_downloadSavePath = UserInfo["downloadSavePath"].GetString();
				}
				if (UserInfo.HasMember("loginCookie") && UserInfo["loginCookie"].IsString())
				{
					GetInstance()->strCookies = UserInfo["loginCookie"].GetString();
				}
			}
		}
		if (!GetInstance()->strCookies.empty())
		{
			BaiduUserInfo baiduInfo;
			ZeroMemory(&baiduInfo, sizeof(BaiduUserInfo));
			if (GetInstance()->m_BaiduPare.GetloginBassInfo(baiduInfo, GetInstance()->strCookies))
			{
				if (!baiduInfo.bdstoken.empty() && !baiduInfo.strUserName.empty())
				{
					GetInstance()->isLogin = true;
					/*
					已经登录百度让登录百度的按钮隐藏
					*/
					wkeRunJS(app.window, "app.tablelistisShwo = true;");
					jsExecState es = wkeGlobalExec(app.window);
					jsValue thisObject = jsGetGlobal(es, "app");
					bool b = jsIsObject(thisObject);
					jsValue func = jsGet(es, thisObject, "updateBaiduList");
					std::string argJson = GetInstance()->m_BaiduPare.GetBaiduFileListInfo("/", GetInstance()->strCookies);
					//argJson = Utf8_To_Gbk(argJson.c_str());
					jsValue jsArg[1] = { jsString(es, argJson.c_str()) };
					jsCall(es, func, thisObject, jsArg, 1);
					result = jsInt(1);
				}
			}
		}
	}
	return result;
}

jsValue CWkeWindow::AraiaPauseStartRemove(jsExecState es, void* param)
{
	if (!param)return jsUndefined();
	jsValue result = jsInt(0);
	//获取参数的数量
	int argCount = jsArgCount(es);
	if (argCount < 2)
		return jsUndefined();
	jsType type = jsArgType(es, 0);
	if (JSTYPE_STRING != type)
		return jsUndefined();
	type = jsArgType(es, 1);
	if (JSTYPE_STRING != type)
		return jsUndefined();
	jsValue arg0 = jsArg(es, 0);
	jsValue arg1 = jsArg(es, 1);
	std::string arg0str = jsToTempString(es, arg0);
	std::string arg1str = jsToTempString(es, arg1);
	if (arg0str == ARIA2_STATUS_ACTIVE)
	{
		std::string strSendText = str(boost::format(ARIA2_UNPAUSE) % arg1str);
		GetInstance()->SendText(GetInstance()->m_BaiduPare.Gbk_To_Utf8(strSendText.c_str()));
	}else if (arg0str == ARIA2_STATUS_PAUSED)
	{
		std::string strSendText = str(boost::format(ARIA2_PAUSE) % arg1str);
		GetInstance()->SendText(GetInstance()->m_BaiduPare.Gbk_To_Utf8(strSendText.c_str()));
	}else if (arg0str == ARIA2_STATUS_REMOVED)
	{
		std::string strSendText = str(boost::format(ARIA2_REMOVE) % arg1str);
		GetInstance()->SendText(GetInstance()->m_BaiduPare.Gbk_To_Utf8(strSendText.c_str()));
	}
	else if (arg0str == "allactive")
	{
		GetInstance()->SendText(GetInstance()->m_BaiduPare.Gbk_To_Utf8(ARIA2_UNAUSEALL));
	}
	else if (arg0str == "allpaused")
	{
		GetInstance()->SendText(GetInstance()->m_BaiduPare.Gbk_To_Utf8(ARIA2_FORCEPAUSEALL));
	}
	return jsInt(1);
}

jsValue CWkeWindow::SwitchDirPath(jsExecState es, void* param)
{
	if (!param)return jsUndefined();
	jsValue result = jsInt(0);
	//获取参数的数量
	int argCount = jsArgCount(es);
	if (argCount < 1)
		return jsUndefined();
	jsType type = jsArgType(es, 0);
	if (JSTYPE_STRING != type)
		return jsUndefined();
	jsValue arg0 = jsArg(es, 0);
	std::string arg0str = jsToTempString(es, arg0);
	//jsExecState es = wkeGlobalExec(app.window);
	arg0str = GetInstance()->m_BaiduPare.Utf8_To_Gbk(arg0str.c_str());
	jsValue thisObject = jsGetGlobal(es, "app");
	bool b = jsIsObject(thisObject);
	jsValue func = jsGet(es, thisObject, "updateBaiduList");
	std::string argJson = GetInstance()->m_BaiduPare.GetBaiduFileListInfo(arg0str, GetInstance()->strCookies);
	jsValue jsArg[1] = { jsString(es, argJson.c_str()) };
	jsCall(es, func, thisObject, jsArg, 1);
	result = jsInt(1);
	return result;
}

jsValue CWkeWindow::DownloadUserFile(jsExecState es, void* param)
{
	if (!param)return jsUndefined();
	//获取参数的数量
	int argCount = jsArgCount(es);
	if (argCount < 1)
		return jsUndefined();
	jsType type = jsArgType(es, 0);
	if (JSTYPE_STRING != type)
		return jsUndefined();
	jsValue arg0 = jsArg(es, 0);
	std::string arg0str = jsToTempString(es, arg0);
	//arg0str = GetInstance()->m_BaiduPare.Utf8_To_Gbk(arg0str.c_str());
	auto proc = [arg0str]() {
		GetInstance()->DownloadUserLocalFile(arg0str);
	};
	std::thread DownloadProc(proc);
	DownloadProc.detach();
	return jsUndefined();
}

jsValue CWkeWindow::ShareBaiduFile(jsExecState es, void* param)
{
	if (!param)return jsUndefined();
	int argCount = jsArgCount(es);
	if (argCount < 3)
		return jsUndefined();
	jsType type = jsArgType(es, 0);
	if (JSTYPE_STRING != type)
		return jsUndefined();
	jsValue arg0 = jsArg(es, 0);
	std::string arg0str = jsToTempString(es, arg0);
	std::string strRetResult;
	SHAREFILEINFO shareinfo;
	if (arg0str == "1") //私密分享
	{
		type = jsArgType(es, 1);
		if (JSTYPE_NUMBER !=type)
			return jsString(es, strRetResult.c_str());
		jsValue arg1 = jsArg(es, 1);
		int period = jsToInt(es, arg1);
		type = jsArgType(es, 2);
		if (JSTYPE_STRING != type)
			return jsString(es, strRetResult.c_str());
		jsValue arg2 = jsArg(es, 2);
		std::string fid_list = jsToTempString(es, arg2);
		type = jsArgType(es, 3);
		if (JSTYPE_STRING != type)
			return jsString(es, strRetResult.c_str());
		jsValue arg3 = jsArg(es, 3);
		std::string pwd = jsToTempString(es, arg3);
		switch (period)
		{
		case 0: //永久分享
		{
					shareinfo.nschannel = 1;
					shareinfo.strpath_list = fid_list;
					shareinfo.strPwd = pwd;
					shareinfo.strperiod = std::to_string(period);
					strRetResult = GetInstance()->m_BaiduPare.ShareBaiduFile(shareinfo, GetInstance()->strCookies);
		}
		break;
		case 7:		//7天分享
		{
						shareinfo.nschannel = 1;
						shareinfo.strpath_list = fid_list;
						shareinfo.strPwd = pwd;
						shareinfo.strperiod = std::to_string(period);
						strRetResult = GetInstance()->m_BaiduPare.ShareBaiduFile(shareinfo, GetInstance()->strCookies);
		}
		break;
		case 1:  //1天分享
		{
					 shareinfo.nschannel = 1;
					 shareinfo.strpath_list = fid_list;
					 shareinfo.strPwd = pwd;
					 shareinfo.strperiod = std::to_string(period);
					 strRetResult = GetInstance()->m_BaiduPare.ShareBaiduFile(shareinfo, GetInstance()->strCookies);
		}
		break;
		default:
			break;
		}
	}else if (arg0str == "2") //公开分享
	{
		type = jsArgType(es, 1);
		if (JSTYPE_NUMBER != type)
			return jsString(es, strRetResult.c_str());
		jsValue arg1 = jsArg(es, 1);
		int period = jsToInt(es, arg1);
		type = jsArgType(es, 2);
		if (JSTYPE_STRING != type)
			return jsString(es, strRetResult.c_str());
		jsValue arg2 = jsArg(es, 2);
		std::string fid_list = jsToTempString(es, arg2);
		switch (period)
		{
		case 0: //永久分享
		{
					shareinfo.nschannel = 0;
					shareinfo.strpath_list = fid_list;
					shareinfo.strperiod = std::to_string(period);
					strRetResult = GetInstance()->m_BaiduPare.ShareBaiduFile(shareinfo, GetInstance()->strCookies);
		}
			break;
		case 7:		//7天分享
		{
						shareinfo.nschannel = 0;
						shareinfo.strpath_list = fid_list;
						shareinfo.strperiod = std::to_string(period);
						strRetResult = GetInstance()->m_BaiduPare.ShareBaiduFile(shareinfo, GetInstance()->strCookies);
		}
			break;
		case 1:  //1天分享
		{
					 shareinfo.nschannel = 0;
					 shareinfo.strpath_list = fid_list;
					 shareinfo.strperiod = std::to_string(period);
					 strRetResult = GetInstance()->m_BaiduPare.ShareBaiduFile(shareinfo, GetInstance()->strCookies);
		}
			break;
		default:
			break;
		}
	}
	return jsString(es, strRetResult.c_str());
}

jsValue CWkeWindow::DeleteBaiduFile(jsExecState es, void* param)
{
	if (!param)return jsUndefined();
	int argCount = jsArgCount(es);
	if (argCount < 1)
		return jsUndefined();
	jsType type = jsArgType(es, 0);
	if (JSTYPE_STRING != type)
		return jsUndefined();
	jsValue arg0 = jsArg(es, 0);
	std::string arg0str = jsToTempString(es, arg0);
	std::string argJson = GetInstance()->m_BaiduPare.DeleteBaiduFile(arg0str, GetInstance()->strCookies);
	jsValue thisObject = jsGetGlobal(es, "app");
	jsValue func = jsGet(es, thisObject, "updateBaiduList");
	jsValue jsArg[1] = { jsString(es, argJson.c_str()) };
	jsCall(es, func, thisObject, jsArg, 1);
	return jsUndefined();
}

jsValue CWkeWindow::DownShareFile(jsExecState es, void* param)
{
	jsValue bResult = jsInt(0);
	if (!param)return jsUndefined();
	int argCount = jsArgCount(es);
	if (argCount < 1)
		return bResult;
	jsType type = jsArgType(es, 0);
	if (JSTYPE_STRING != type)
		return bResult;
	jsValue arg0 = jsArg(es, 0);
	std::string arg0str = jsToTempString(es, arg0);
	int nType = GetInstance()->JudgeDownUrlType(arg0str);
	switch (nType)
	{
	case 1: /*百度网盘*/
	{
				std::string strTempCookie(GetInstance()->strCookies);
				if (strTempCookie.empty())
					return bResult;
				//优化这里可以使用线程不然卡界面
				auto BaiduDownloadThreadProc = [arg0str](std::string& TempCookie) {
					REQUESTINFO rResult;
					std::string strJson;
					ZeroMemory(&rResult, sizeof(REQUESTINFO));
					rResult = GetInstance()->m_BaiduPare.ParseBaiduAddr(arg0str.c_str(), TempCookie);
					rResult.strFileName = GetInstance()->m_BaiduPare.Utf8_To_Gbk(rResult.strFileName.c_str());
					GetInstance()->AddShareFileItem(rResult);
					HttpRequest BaiduHttp;
					BaiduHttp.SetRequestHeader("Connection", "keep-alive");
					BaiduHttp.SetRequestHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
					BaiduHttp.SetRequestHeader("Upgrade-Insecure-Requests", "1");
					BaiduHttp.SetRequestHeader("Referer", arg0str);
					std::string strHost = GetInstance()->CarkUrl(rResult.strDownloadUrl.c_str());
					int nPos = strHost.find("//");
					if (nPos!=std::string::npos)
					{
						strHost = strHost.substr(nPos + 2, strHost.length() - (nPos + 2));
					}
					BaiduHttp.SetRequestHeader("Host", strHost);
					BaiduHttp.SetRequestCookies(rResult.strCookies);
					BaiduHttp.Send(HEAD, rResult.strDownloadUrl);
					rResult.strDownloadUrl = GetInstance()->m_BaiduPare.GetTextMid(BaiduHttp.GetallResponseHeaders(), "Location: ", "\r\n");
					rResult.strSavePath = "d:/pdf";
					strJson = GetInstance()->CreateDowndAria2Json(rResult);
					GetInstance()->SendText(strJson);
				};
				std::thread DownThread(BaiduDownloadThreadProc, strTempCookie);
				DownThread.detach();
				bResult = jsInt(1);
	}
	break;
	case 2: /*蓝奏网盘*/
	{

	}
	break;
	case 3:/*城通网盘*/
	{

	}
	break;
	default:
	{
			   REQUESTINFO rResult;
			   std::string strJson;
			   ZeroMemory(&rResult, sizeof(REQUESTINFO));
			   rResult.strDownloadUrl = arg0str;
			   rResult.strSavePath = "d:/pdf";
			   strJson = GetInstance()->CreateDowndAria2Json(rResult);
			   GetInstance()->SendText(strJson);
			   bResult = jsInt(1);
	}
		break;
	}
	return bResult;
}

int CWkeWindow::JudgeDownUrlType(const std::string& strUrl)
{
	int nResult = 0;
	if (strUrl.empty())
		return nResult;
	std::string StrHost;
	StrHost = CarkUrl(strUrl.c_str());
	if (!_strcmpi(StrHost.c_str(), "http://pan.baidu.com"))
		nResult = 1;
	else if (!_strcmpi(StrHost.c_str(), "http://www.lanzous.com"))
		nResult = 2;
	else if (StrHost.find("ctfile.com")!=std::string::npos)
	{
		nResult = 3;
	}
	return nResult;
}

std::string CWkeWindow::CreateDowndAria2Json(REQUESTINFO& Downdinfo)
{
	std::string strResultJson;
	if (Downdinfo.strDownloadUrl.empty())
		return strResultJson;
	rapidjson::Document dc;
	dc.SetObject();
	rapidjson::Value Method(rapidjson::kStringType);
 	const char method[] = "aria2.addUri";
	Method.SetString(method, strlen(method), dc.GetAllocator());
	dc.AddMember(rapidjson::StringRef("method"), Method, dc.GetAllocator());
	rapidjson::Value Params(rapidjson::kArrayType);
	rapidjson::Value token(rapidjson::kStringType);
	token.SetString("token:CDP", strlen("token:CDP"), dc.GetAllocator());
	Params.PushBack(token, dc.GetAllocator());
	rapidjson::Value downloadUrlList(rapidjson::kArrayType);
	for (size_t i = 0; i < 1;i++)
	{
		rapidjson::Value downloadUrl(rapidjson::kStringType);
		downloadUrl.SetString(Downdinfo.strDownloadUrl.c_str(), Downdinfo.strDownloadUrl.length(), dc.GetAllocator());
		downloadUrlList.PushBack(downloadUrl,dc.GetAllocator());
	}
	Params.PushBack(downloadUrlList,dc.GetAllocator());
	rapidjson::Value itemObj(rapidjson::kObjectType);
	rapidjson::Value out(rapidjson::kStringType);
	out.SetString(Downdinfo.strFileName.c_str(), Downdinfo.strFileName.length(), dc.GetAllocator());
	itemObj.AddMember(rapidjson::StringRef("out"), out, dc.GetAllocator());
	rapidjson::Value dir(rapidjson::kStringType);
	dir.SetString(Downdinfo.strSavePath.c_str(), Downdinfo.strSavePath.length(), dc.GetAllocator());
	itemObj.AddMember(rapidjson::StringRef("dir"), dir, dc.GetAllocator());
	rapidjson::Value user_agent(rapidjson::kStringType);
	if (Downdinfo.strUserAegnt.empty())
	{
		Downdinfo.strUserAegnt = USER_AGENT;
	}
	user_agent.SetString(Downdinfo.strUserAegnt.c_str(), Downdinfo.strUserAegnt.length(), dc.GetAllocator());
	itemObj.AddMember(rapidjson::StringRef("user-agent"), user_agent, dc.GetAllocator());
	rapidjson::Value max_tries(rapidjson::kStringType);
	max_tries.SetString("10", strlen("10"), dc.GetAllocator());
	itemObj.AddMember(rapidjson::StringRef("max-tries"), max_tries, dc.GetAllocator());
	rapidjson::Value timeout(rapidjson::kStringType);
	timeout.SetString("5", strlen("5"), dc.GetAllocator());
	itemObj.AddMember(rapidjson::StringRef("timeout"), timeout, dc.GetAllocator());
	rapidjson::Value connect_timeout(rapidjson::kStringType);
	connect_timeout.SetString("5", strlen("5"), dc.GetAllocator());
	itemObj.AddMember(rapidjson::StringRef("connect-timeout"), connect_timeout, dc.GetAllocator());
	rapidjson::Value split(rapidjson::kStringType);
	split.SetString("128", strlen("128"), dc.GetAllocator());
	itemObj.AddMember(rapidjson::StringRef("split"), split, dc.GetAllocator());
	rapidjson::Value retry_wait(rapidjson::kStringType);
	retry_wait.SetString("10", strlen("10"), dc.GetAllocator());
	itemObj.AddMember(rapidjson::StringRef("retry-wait"), retry_wait, dc.GetAllocator());
	Params.PushBack(itemObj, dc.GetAllocator());
	dc.AddMember(rapidjson::StringRef("params"), Params, dc.GetAllocator());
	rapidjson::Value id(rapidjson::kNumberType);
	id.SetInt(1);
	dc.AddMember(rapidjson::StringRef("id"), id, dc.GetAllocator());
	rapidjson::Value jsonrpc(rapidjson::kStringType);
	jsonrpc.SetString("2.0", strlen("2.0"), dc.GetAllocator());
	dc.AddMember(rapidjson::StringRef("jsonrpc"), jsonrpc, dc.GetAllocator());
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	dc.Accept(writer);
	strResultJson = buffer.GetString();
	strResultJson = m_BaiduPare.Gbk_To_Utf8(strResultJson.c_str());
	return strResultJson;
}
std::string& CWkeWindow::replace_all_distinct(std::string& str, const std::string& old_value, const std::string& new_value)
{
	for (std::string::size_type pos(0); pos != std::string::npos; pos += new_value.length()) {
		if ((pos = str.find(old_value, pos)) != std::string::npos)
			str.replace(pos, old_value.length(), new_value);
		else   break;
	}
	return   str;
}
bool CWkeWindow::DownloadUserLocalFile(const std::string& strJsonData)
{
	bool bResult = false;
	const std::string& strCookie = GetInstance()->strCookies;
	if (strJsonData.empty())
	{
		return bResult;
	}
	rapidjson::Document dc;
	dc.Parse(strJsonData.c_str());
	if (!dc.IsObject())
		return bResult;
	if (dc.HasMember("data") && dc["data"].IsArray())
	{
		for (auto& v : dc["data"].GetArray())
		{
			if (v.HasMember("isdir") && v["isdir"].IsUint() && v.HasMember("path") && v["path"].IsString())
			{
				UINT nIsdir = v["isdir"].GetUint();
				std::string strPath = v["path"].GetString();
				strPath = m_BaiduPare.Utf8_To_Gbk(strPath.c_str());
				std::string strResultJson;
				if (nIsdir)
				{
					strResultJson = m_BaiduPare.GetBaiduFileListInfo(strPath, strCookie);
					DownloadUserLocalFile(strResultJson);
				}
				else
				{
					//OutputDebugStringA(strPath.c_str());
					strResultJson = m_BaiduPare.GetBaiduLocalFileAddr(m_BaiduPare.URL_Coding(strPath.c_str()), strCookie);
					if (!strResultJson.empty())
					{
						REQUESTINFO rResult;
						std::string strJson;
						ZeroMemory(&rResult, sizeof(REQUESTINFO));
						rResult.strDownloadUrl = strResultJson;
						rResult.strCookies = strCookies;
						rResult.strFileName = strPath;
						rResult.strSavePath = "d:/pdf";
						rResult.strUserAegnt = NETDISK_USER_AGENT;
						std::string strAddurl = CreateDowndAria2Json(rResult);
						if (!strAddurl.empty())
						{
							SendText(strAddurl);
						}
					}
					//CreateDowndAria2Json
				}
			}
		}
	}
	return bResult;
}

CWkeWindow::CWkeWindow()
:isLogin(false),
strCookies(""),
m_hwnd(NULL),
numActive(NULL),
numStopped(NULL),
numWaiting(NULL),
m_downloadSavePath("")
{
	try
	{
		if (!InintMiniBlink())
		{
			throw std::logic_error("Environment initialization failed");
		}
	}
	catch (...)
	{
		//可以打个日志直接结束程序
		LOG(INFO) << "初始化miniblink环境失败";
		exit(0);
	}
#if 1
	/*
	初始化ASIO
	*/
	m_WssClient.init_asio();
	// Register our handlers
	m_WssClient.set_socket_init_handler(bind(&CWkeWindow::on_socket_init, this, ::_1));
	m_WssClient.set_message_handler(bind(&CWkeWindow::on_message, this, ::_1, ::_2));
	m_WssClient.set_open_handler(bind(&CWkeWindow::on_open, this, ::_1));
	m_WssClient.set_close_handler(bind(&CWkeWindow::on_close, this, ::_1));
	m_WssClient.set_fail_handler(bind(&CWkeWindow::on_fail, this, ::_1));
#endif
	if (RunAria2())
	{
		m_RunThreadPtr.reset(new std::thread(&CWkeWindow::Connect, this));
	}
}

CWkeWindow::~CWkeWindow()
{
	if (m_RunThreadPtr)
	{
		if (m_RunThreadPtr->joinable())
		{
			if(m_hdl.lock().get())
			{
				m_WssClient.close(m_hdl, websocketpp::close::status::going_away, "close");
			}
			m_RunThreadPtr->join();
		}
		m_RunThreadPtr.reset();
	}
}
CWkeWindow* CWkeWindow::GetInstance()
{
	if (Instance == NULL)
	{
		g_mutx.lock();
		if (!Instance.get())
		{
			//std::shared_ptr<CWkeWindow> temp = std::shared_ptr<CWkeWindow>(new CWkeWindow);
			Instance.reset(new CWkeWindow);
		}
		g_mutx.unlock();
	}
	return Instance.get();
}

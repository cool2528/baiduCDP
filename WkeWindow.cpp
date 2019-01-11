#include "WkeWindow.h"
#include <shlwapi.h>
#include <WinInet.h>
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
			//开始组装json发送给UI线程处理渲染到界面
			rapidjson::Document sendJson;
			sendJson.SetObject();
			rapidjson::Value arrlist(rapidjson::kArrayType);
			int nErrcount = 0;
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
				if (v.HasMember("errorMessage") && v["errorMessage"].IsString() && v.HasMember("errorCode") && v["errorCode"].IsString())
				{
					nErrcount++;
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
			if (!nErrcount)
			{
				::PostMessage(m_hwnd, ARIA2_UPDATE_TELLACTIVE_LIST_MSG, NULL, (LPARAM)pSendDataPtr);
			}

		}
		if (numStopped)
		{
			if (!m_ErrorArray.empty())
			{
				if (dc.HasMember("result") && dc["result"].IsArray())
				{
					//开始组装json发送给UI线程处理渲染到界面
					rapidjson::Document sendJson;
					sendJson.SetObject();
					rapidjson::Value arrlist(rapidjson::kArrayType);
					int nErrcount = 0;
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
						if (v.HasMember("errorMessage") && v["errorMessage"].IsString() && v.HasMember("errorCode") && v["errorCode"].IsString())
						{
							nErrcount++;
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
					if (nErrcount)
						::PostMessage(m_hwnd, ARIA2_UPDATE_TELLERROR_LIST_MSG, NULL, (LPARAM)pSendDataPtr);
			}
		}
	}
	if (dc.HasMember("result") && dc["result"].IsObject())
	{
		rapidjson::Value itemObj = dc["result"].GetObjectW();
		if (itemObj.HasMember("numActive") && itemObj["numActive"].IsString())
			numActive = atol(itemObj["numActive"].GetString());
		if (itemObj.HasMember("numStopped") && itemObj["numStopped"].IsString())
			numStopped = atol(itemObj["numStopped"].GetString());
	}
}

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
			updateErrorList(strJsonData);
			GetInstance()->m_ErrorArray.clear();
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
			auto it = GetInstance()->m_DownloadArray.begin();
			for (size_t i=0; i < GetInstance()->m_DownloadArray.size();i++)
			{
				if (GetInstance()->m_DownloadArray[i].strFileGid == strGID)
					GetInstance()->m_DownloadArray.erase(it+i);
			}
			strGID = GetInstance()->GetFileCompletedInfo(strGID);
			//更新数据到已下载完成列表
			static auto addSussedList = [](const std::string Gid) {
				std::string buffer = str(boost::format("{\"name\":\"%1%\",\"ChangeTime\":\"%2%\"}") % Gid % GetInstance()->m_BaiduPare.timestampToDate((ULONGLONG)time(NULL)));
				jsExecState es = wkeGlobalExec(app.window);
				jsValue thisObject = jsGetGlobal(es, "app");
				jsValue func = jsGet(es, thisObject, "updatedownloadSussedList");
				jsValue jsArg[1] = { jsString(es, buffer.c_str()) };
				jsCall(es, func, thisObject, jsArg, 1);
			};
			addSussedList(strGID);
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
			DOWNLOADSTATUS vector_item;
			ZeroMemory(&vector_item, sizeof(DOWNLOADSTATUS));
			vector_item.strFileGid = strGID;
			vector_item.strFileStatus = ARIA2_STATUS_ACTIVE;
			GetInstance()->m_DownloadArray.push_back(vector_item);
		}
	}
	break;
	case ARIA2_DOWNLOAD_STOP_MSG: //某个任务停止下载
	{

	}
	break;
	case ARIA2_DOWNLOAD_PAUSE_MSG: //某个任务 暂停下载
	{
									   index++;
	}
	break;
	case ARIA2_DOWNLOAD_ERROR_MSG: //某个任务下载出错
	{
			if (lParam)
			{
				const char* pbuffer = (const char*)lParam;
				std::string strGID(pbuffer);
				delete pbuffer;
				//清除下载失败的文件内存缓存
				std::string strformat = str(boost::format(ARIA2_PURGEDOWNLOAD_RESULT) % strGID);
				GetInstance()->SendText(GetInstance()->m_BaiduPare.Gbk_To_Utf8(strformat.c_str()));
				if (GetInstance()->m_ErrorArray.empty())
				{
					GetInstance()->m_ErrorArray.push_back(strGID);
				}
				else
				{
					for (size_t i = 0; i < GetInstance()->m_ErrorArray.size(); i++)
					{
						if (GetInstance()->m_ErrorArray.at(i) != strGID)
						{
							GetInstance()->m_ErrorArray.push_back(strGID);
						}
					}
				}
			}
	}
	break;
	default:
		break;
	}
	return CallWindowProc(m_oldProc, hwnd, uMsg, wParam, lParam);
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
		//启动失败
		return;
	}
	m_WssClient.connect(con);
	m_WssClient.run();
}

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

void CWkeWindow::Connect()
{
	try{
		start("ws://127.0.0.1:6800/jsonrpc");
	}
	catch (websocketpp::exception const & e) {
		std::cout << e.what() << std::endl;
	}
	catch (std::exception const & e) {
		std::cout << e.what() << std::endl;
	}
	catch (...) {
		std::cout << "other exception" << std::endl;
	}
}

void CWkeWindow::SendText(std::string& strText)
{
	if (m_hdl.lock().get())
	{
		m_WssClient.send(m_hdl, strText, websocketpp::frame::opcode::text);
	}
}

void CALLBACK CWkeWindow::TimeProc(HWND hwnd, UINT message, UINT iTimerID, DWORD dwTime)
{
	GetInstance()->SendText(GetInstance()->m_BaiduPare.Gbk_To_Utf8(ARIA2_GETGLOBAL_STATUS));
	if (GetInstance()->numActive > 0)
	{
		GetInstance()->SendText(GetInstance()->m_BaiduPare.Gbk_To_Utf8(ARIA2_TELLACTICE_SENDDATA));
		wkeRunJS(app.window, "app.backupdownloadListinfo = app.downloadListinfo;");
	}
	else if (GetInstance()->numStopped > 0 && !GetInstance()->m_ErrorArray.empty())
	{
		GetInstance()->SendText(GetInstance()->m_BaiduPare.Gbk_To_Utf8(ARIA2_TELLSTOPPED));
	}
	else
	{
		GetInstance()->numStopped = 0;
	}
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
	app->window = wkeCreateWebWindow(WKE_WINDOW_TYPE_TRANSPARENT, NULL, 0, 0, 1024, 640);
	if (!app->window)
		return false;
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
	std::string localUrl = ((CWkeWindow*)param)->CarkUrl(url);
	bool pos = localUrl == HOST_NAME ? true : false;
	if (pos) {
		const utf8* decodeURL = wkeUtilDecodeURLEscape(url);
		if (!decodeURL)
			return false;
		std::string urlString(decodeURL);
		std::string localPath = urlString.substr(sizeof(HOST_NAME)-1);

		std::wstring path = ((CWkeWindow*)param)->getResourcesPath(((CWkeWindow*)param)->utf8ToUtf16(localPath));
		std::vector<char> buffer;

		((CWkeWindow*)param)->readJsFile(path.c_str(), &buffer);

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
		((CWkeWindow*)param)->isLogin = true;
		((CWkeWindow*)param)->strCookies = wkeGetCookie(webWindow);
		OutputDebugStringA(wkeGetCookie(webWindow));
		/*
		已经登录百度让登录百度的按钮隐藏
		*/
		wkeRunJS(app.window, "app.tablelistisShwo = true;");
		jsExecState es = wkeGlobalExec(app.window);
		jsValue thisObject = jsGetGlobal(es, "app");
		bool b = jsIsObject(thisObject);
		jsValue func = jsGet(es, thisObject, "updateBaiduList");
		std::string argJson = ((CWkeWindow*)param)->m_BaiduPare.GetBaiduFileListInfo("/", ((CWkeWindow*)param)->strCookies);
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
		//wkeSetDebugConfig(app.window, "showDevTools", GetInstance()->m_BaiduPare.Gbk_To_Utf8("E:\\vs2013\\Win32\\BaiduCdp\\Debug\\front_end\\inspector.html").c_str());
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
		((CWkeWindow*)param)->blinkClose();
	else if (arg0str == MAX_MSG)
		((CWkeWindow*)param)->blinkMaximize();
	else if (arg0str == MIN_MSG)
		((CWkeWindow*)param)->blinkMinimize();
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
	if (!((CWkeWindow*)param)->strCookies.empty() && ((CWkeWindow*)param)->isLogin)
		result = jsInt(1);
	return result;
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
	arg0str = ((CWkeWindow*)param)->m_BaiduPare.Utf8_To_Gbk(arg0str.c_str());
	jsValue thisObject = jsGetGlobal(es, "app");
	bool b = jsIsObject(thisObject);
	jsValue func = jsGet(es, thisObject, "updateBaiduList");
	std::string argJson = ((CWkeWindow*)param)->m_BaiduPare.GetBaiduFileListInfo(arg0str, ((CWkeWindow*)param)->strCookies);
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
	arg0str = ((CWkeWindow*)param)->m_BaiduPare.Utf8_To_Gbk(arg0str.c_str());
	arg0str = ((CWkeWindow*)param)->m_BaiduPare.URL_Coding(arg0str.c_str());
	arg0str = ((CWkeWindow*)param)->m_BaiduPare.GetBaiduLocalFileAddr(arg0str, ((CWkeWindow*)param)->strCookies);
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
					strRetResult = ((CWkeWindow*)param)->m_BaiduPare.ShareBaiduFile(shareinfo, ((CWkeWindow*)param)->strCookies);
		}
		break;
		case 7:		//7天分享
		{
						shareinfo.nschannel = 1;
						shareinfo.strpath_list = fid_list;
						shareinfo.strPwd = pwd;
						shareinfo.strperiod = std::to_string(period);
						strRetResult = ((CWkeWindow*)param)->m_BaiduPare.ShareBaiduFile(shareinfo, ((CWkeWindow*)param)->strCookies);
		}
		break;
		case 1:  //1天分享
		{
					 shareinfo.nschannel = 1;
					 shareinfo.strpath_list = fid_list;
					 shareinfo.strPwd = pwd;
					 shareinfo.strperiod = std::to_string(period);
					 strRetResult = ((CWkeWindow*)param)->m_BaiduPare.ShareBaiduFile(shareinfo, ((CWkeWindow*)param)->strCookies);
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
					strRetResult = ((CWkeWindow*)param)->m_BaiduPare.ShareBaiduFile(shareinfo, ((CWkeWindow*)param)->strCookies);
		}
			break;
		case 7:		//7天分享
		{
						shareinfo.nschannel = 0;
						shareinfo.strpath_list = fid_list;
						shareinfo.strperiod = std::to_string(period);
						strRetResult = ((CWkeWindow*)param)->m_BaiduPare.ShareBaiduFile(shareinfo, ((CWkeWindow*)param)->strCookies);
		}
			break;
		case 1:  //1天分享
		{
					 shareinfo.nschannel = 0;
					 shareinfo.strpath_list = fid_list;
					 shareinfo.strperiod = std::to_string(period);
					 strRetResult = ((CWkeWindow*)param)->m_BaiduPare.ShareBaiduFile(shareinfo, ((CWkeWindow*)param)->strCookies);
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
	std::string argJson = ((CWkeWindow*)param)->m_BaiduPare.DeleteBaiduFile(arg0str, ((CWkeWindow*)param)->strCookies);
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
	int nType = ((CWkeWindow*)param)->JudgeDownUrlType(arg0str);
	switch (nType)
	{
	case 1: /*百度网盘*/
	{
				REQUESTINFO rResult;
				std::string strJson;
				ZeroMemory(&rResult, sizeof(REQUESTINFO));
				std::string strTempCookie(((CWkeWindow*)param)->strCookies);
				if (strTempCookie.empty())
					return bResult;
				//优化这里可以使用线程不然卡界面
				rResult = ((CWkeWindow*)param)->m_BaiduPare.ParseBaiduAddr(arg0str.c_str(), strTempCookie);
				rResult.strSavePath = "d:/pdf";
				rResult.strFileName = GetInstance()->m_BaiduPare.Utf8_To_Gbk(rResult.strFileName.c_str());
				strJson = ((CWkeWindow*)param)->CreateDowndAria2Json(rResult);
				((CWkeWindow*)param)->SendText(strJson);
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
			   strJson = ((CWkeWindow*)param)->CreateDowndAria2Json(rResult);
			   ((CWkeWindow*)param)->SendText(strJson);
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
	for (size_t i = 0; i < 18;i++)
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
	user_agent.SetString(USER_AGENT, strlen(USER_AGENT), dc.GetAllocator());
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

CWkeWindow::CWkeWindow()
:isLogin(false),
strCookies(""),
m_hwnd(NULL),
numActive(NULL),
numStopped(NULL)
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
		exit(0);
	}
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

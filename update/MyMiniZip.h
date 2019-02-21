#ifndef __MYMINIZIP__
#define __MYMINIZIP__
/*
@ ZLIB 实现简单的zip文件压缩|解压缩功能
@ 调用 ZLIB
@ http://www.zlib.net/
@ 2018年12月17日
*/
#include <windows.h>
#include <string>
#include <chrono>
extern "C"
{
#include "zlib.h"
#include "zconf.h"
#include "unzip.h"
#include "zip.h"
}
#if 0
/*
@ 获取函数执行耗时多少秒
*/
typedef std::chrono::steady_clock::time_point Time;
class MyCTime
{
public:
	MyCTime();
	~MyCTime();
	Time start() const;
	Time end() const;
	int CountInterval(Time& begin, Time& end);
private:

};
#endif
/*
@ 核心类 处理 压缩解压缩 
*/
class MyMiniZip
{
public:
	explicit MyMiniZip();
	~MyMiniZip();
public:
	/*
	@ 获取unZipPackageToLoacal 函数解压共计用时
	*/
	int GetCountTime() const;
	/*
	@ 获取当前解压文件或者压缩文件的状态信息
	*/
	std::string GetFileStatus() const;
	/*
	@ 获取压缩包内全局注释文本
	*/
	std::string GetGlobalComment() const;
	/*
	@ 压缩文件 或者文件夹 内部自动判断
	@ strSourceFile 要被压缩的文件或文件路径全称
	@ strSaveFile 要生成的zip压缩包名全路径
	@ 失败返回 false 成功返回 true
	*/
	bool CompressToPackageZip(const std::string strSourceFile, const std::string strSaveFile);
	/*
	@ 解压zip 包文件到指定路径
	@ strSourceZipPath 要被解压的zip文件包全路径
	@ strDestZipPath 将要解压到的本地路径
	@ 成功返回 解压文件的数量 失败返回 null
	*/
	DWORD unZipPackageToLoacal(const std::string strSourceZipPath, const std::string strDestZipPath);
private:
	/*
	@ 将文件或者文件夹添加到zip压缩包中
	@ ZipFile zipFile 句柄
	@ strFileInZipName 文件或者文件夹名称
	@ strPath 如果 strPath 为null表示 是文件夹 否则是文件
	*/
	bool addFileZip(zipFile ZipFile, const std::string strFileInZipName, const std::string strPath);
	/*
	 @ 递归式遍历文件夹下所有文件并添加到zip压缩包
	 @ ZipFile zipFile 句柄
	 @ strFileInZipName 文件或者文件夹名称
	 @ strPath 如果 strPath 为null表示 是文件夹 否则是文件
	*/
	bool  packFolderToZip(zipFile ZipFile, const std::string strFileInZipName, const std::string strPath);
	/*
	@ 压缩文件 或者文件夹 内部自动判断
	@ strSourceFile 要被压缩的文件或文件路径全称
	@ strSaveFile 要生成的zip压缩包名全路径
	@ nMode 表示压缩文件还是文件夹 0 是文件 1 是文件夹
	@ 失败返回 false 成功返回 true
	*/
	bool CompressToPackageZip(const std::string strSourceFile, const std::string strSaveFile,int nMode);
	/*
	@ 从指定路径读取二进制文件
	@ szFileName 要读取的文件全路径
	@ pFileBuffer 用来接收读取文件的内存buffer
	@ 返回读取到文件的字节大小 失败返回null
	*/
	DWORD ReadFileBuffer(std::string szFileName, PVOID* pFileBuffer);
	/*
	@ 从内存中写文件数据到本地
	@ szFileNmae 写出文件的全路径
	@ pFileBuffer 写出文件的数据指针
	@ dwFileSize 欲写出文件的大小长度
	@ 失败返回 -1 成功 返回实际写出文件大小
	*/
	DWORD WriteFileBuffer(std::string szFileNmae, PVOID pFileBuffer, DWORD dwFileSize);
private:
// 	Time TimeBegin;
// 	Time TimeEnd;
	int nCountTime;
	/*
	@ 当前操作解压文件状态
	*/
	std::string file_status;
	/*
	@ 压缩包内全局注释文本
	*/
	std::string global_comment;
	/*
	@ 解压ZIP包的 句柄
	*/
	unzFile unZipFileHandle;
};

#endif
#ifndef __CENCRYTPION__H
#define __CENCRYTPION__H
#include <windows.h>
#include <string>
namespace aip {
	std::string base64_encode(const char * bytes_to_encode, unsigned int in_len);
	std::string base64_decode(std::string const & encoded_string);
}

typedef enum {
	GENERAL = 0,
	ECB,
	CBC,
	CFB,
	OFB,
	TRIPLE_ECB,
	TRIPLE_CBC
}CRYPTO_MODE;
class CCEncryption
{
public:
	CCEncryption();
	~CCEncryption();
	/////////////////////////////////////////////////////////
	/*						MD5加密函数						*/
	/////////////////////////////////////////////////////////
	/*
	@返回字符串的MD5值
	*/
	static std::string MD5_Str(const std::string strData);
	/*
	@返回内存数据的MD5值
	*/
	static std::string Md5_Memory(PVOID pBuffer, DWORD dwlens);
	/*
	@ 根据文件路径读取文件返回文件的MD5值
	*/
	static std::string Md5_Files(const std::string strFiles);
	/////////////////////////////////////////////////////////
	/*						DES加解密函数					*/
	/////////////////////////////////////////////////////////
	/*
	@ DES加密
	@ cleartext 被加解密文本
	@ key 密匙串
	@ mode 加解密方式
	*/
	static std::string DES_Encrypt(const std::string cleartext, const std::string key, CRYPTO_MODE mode);
	/*
	@ DES解密
	@ ciphertext 被加解密文本
	@ key 密匙串
	@ mode 加解密方式
	*/
	static std::string DES_Decrypt(const std::string ciphertext, const std::string key, CRYPTO_MODE mode);
	/////////////////////////////////////////////////////////
	/*						AES加解密函数					*/
	/////////////////////////////////////////////////////////
	/*
	@ AES加密函数
	@ strKey 加密用的密匙
	@ strData 被加密的数据
	@ mode  加密的方式
	*/
	static std::string AES_Encrypt(const std::string strKey, const std::string strData);
	/*
	@ AES解密函数
	@ strKey 解密用的密匙
	@ strData 被解密的数据
	@ mode  解密的方式 
	*/
	static std::string AES_Decrypt(const std::string strKey, const std::string strData);
	static std::string Gbk_To_Utf8(const char* szBuff);
private:
	#define MD5_LENS 16
	static BYTE cbc_iv[8];
	static std::string HexToStr(PBYTE pBuffer, DWORD dwLens);
	static std::string StrToHex(const std::string strBuffer);
};

#endif
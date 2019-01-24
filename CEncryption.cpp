#include "CEncryption.h"
#include <openssl/md5.h>
#include <openssl/des.h>
#include <openssl/aes.h>
#include <fstream>
#include <vector>
/*#include "base64.h"*/
BYTE CCEncryption::cbc_iv[8] = {0};
std::string CCEncryption::HexToStr(PBYTE pBuffer, DWORD dwLens)
{
	std::string strResult;
	if (nullptr==pBuffer)return strResult;
	CHAR szTemp[3];
	ZeroMemory(szTemp, 3);
	for (int i=0;i<dwLens;i++)
	{
		sprintf_s(szTemp, "%02X", pBuffer[i]);
		strResult += szTemp;
	}
	return strResult;
}

std::string CCEncryption::StrToHex(const std::string strBuffer)
{
	std::string strResult;
	PBYTE pBuffer = nullptr;
	if (strBuffer.empty())return strResult;
	int lens = strBuffer.length();
	pBuffer = (PBYTE)strBuffer.c_str();
	char* pResult = new char[lens];
	ZeroMemory(pResult, lens);
	//96 47 A6 05 53 60 69 26 18 10 2A D3 25 18 43 66 BC 81 B9 A3 0C 4D 8D 11 72 9A AD 3C D1 F5 E1 06 A3 98 99 B7 18 00 94 79
	for (int i = 0,j=0;i<lens;i=i+2,j++)
	{
		DWORD dwCode = NULL;
		sscanf_s((const char*)pBuffer+i, "%02x", &dwCode);
		memcpy(&pResult[j], &dwCode, sizeof(BYTE));
	}
	strResult.assign(pResult);
	delete pResult;
	return strResult;
}

CCEncryption::CCEncryption()
{
}


CCEncryption::~CCEncryption()
{
}

std::string CCEncryption::MD5_Str(const std::string strData)
{
	unsigned char MD5lens[MD5_LENS] = { 0 };
	std::string strResultMd5;
	char szMd5buf[256];
	char szTemp[3];
	memset(szTemp, 0, 3);
	memset(szMd5buf,0, 256);
	MD5((unsigned char*)strData.c_str(), strData.length(), MD5lens);
	for (int i = 0; i < MD5_LENS; i++)
	{
		sprintf_s(szTemp, "%02X", (int)MD5lens[i]);
		strcat_s(szMd5buf, szTemp);
		memset(szTemp, 0, 3);
	}
	strResultMd5 = szMd5buf;
	return strResultMd5;
}

std::string CCEncryption::Md5_Memory(PVOID pBuffer, DWORD dwlens)
{
	MD5_CTX md5_ctx;
	std::string strResultMd5;
	char szMd5buf[256];
	char szTemp[3];
	memset(szTemp, 0, 3);
	memset(szMd5buf, 0, 256);
	MD5_Init(&md5_ctx);
	unsigned char MD5lens[MD5_LENS] = { 0 };
	if (dwlens)
		MD5_Update(&md5_ctx, pBuffer, dwlens);
	else
		return strResultMd5;
	MD5_Final(MD5lens, &md5_ctx);
	for (int i = 0; i < MD5_LENS; i++)
	{
		sprintf_s(szTemp, "%02X", (int)MD5lens[i]);
		strcat_s(szMd5buf, szTemp);
		memset(szTemp, 0, 3);
	}
	strResultMd5 = szMd5buf;
	return strResultMd5;
}

std::string CCEncryption::Md5_Files(const std::string strFiles)
{
	std::string strResultMd5;
	DWORD dwFileAttr = -1;
	if (strFiles.empty())return strResultMd5;
	dwFileAttr = ::GetFileAttributesA(strFiles.c_str());
	if (INVALID_FILE_ATTRIBUTES != dwFileAttr && dwFileAttr | FILE_ATTRIBUTE_DIRECTORY)
	{
		//表示文件是存在的
		std::ifstream iFile(strFiles.c_str(), std::ios::binary | std::ios::in);
		if (!iFile.is_open())
			return strResultMd5;
		std::streamoff lens;
		iFile.seekg(0l,std::ios::end);
		lens = iFile.tellg();
		iFile.seekg(0l,std::ios::beg);
		PCHAR pBuffer = nullptr;
		pBuffer = new CHAR[lens + 1];
		if (!pBuffer)
		{
			iFile.close();
			return strResultMd5;
		}
		ZeroMemory(pBuffer, lens + 1);
		iFile.read(pBuffer, lens);
		iFile.close();
		strResultMd5 = Md5_Memory(pBuffer, lens);
	}
	return strResultMd5;
}

std::string CCEncryption::DES_Encrypt(const std::string cleartext, const std::string key, CRYPTO_MODE mode)
{
	std::string strCipherText;
	switch (mode) {
	case GENERAL:
	case ECB:
	{
		DES_cblock keyEncrypt;
		memset(keyEncrypt, 0, 8);

		if (key.length() <= 8)
			memcpy(keyEncrypt, key.c_str(), key.length());
		else
			memcpy(keyEncrypt, key.c_str(), 8);

		DES_key_schedule keySchedule;
		DES_set_key_unchecked(&keyEncrypt, &keySchedule);

		const_DES_cblock inputText;
		DES_cblock outputText;
		std::vector<unsigned char> vecCiphertext;
		unsigned char tmp[8];

		for (int i = 0; i < cleartext.length() / 8; i++) {
			memcpy(inputText, cleartext.c_str() + i * 8, 8);
			DES_ecb_encrypt(&inputText, &outputText, &keySchedule, DES_ENCRYPT);
			memcpy(tmp, outputText, 8);

			for (int j = 0; j < 8; j++)
				vecCiphertext.push_back(tmp[j]);
		}

		if (cleartext.length() % 8 != 0) {
			int tmp1 = cleartext.length() / 8 * 8;
			int tmp2 = cleartext.length() - tmp1;
			memset(inputText, 0, 8);
			memcpy(inputText, cleartext.c_str() + tmp1, tmp2);

			DES_ecb_encrypt(&inputText, &outputText, &keySchedule, DES_ENCRYPT);
			memcpy(tmp, outputText, 8);

			for (int j = 0; j < 8; j++)
				vecCiphertext.push_back(tmp[j]);
		}

		strCipherText.clear();
		strCipherText.assign(vecCiphertext.begin(), vecCiphertext.end());
	}
	break;
	case CBC:
	{
		DES_cblock keyEncrypt, ivec;
		memset(keyEncrypt, 0, 8);

		if (key.length() <= 8)
			memcpy(keyEncrypt, key.c_str(), key.length());
		else
			memcpy(keyEncrypt, key.c_str(), 8);

		DES_key_schedule keySchedule;
		DES_set_key_unchecked(&keyEncrypt, &keySchedule);

		memcpy(ivec, cbc_iv, sizeof(cbc_iv));

		int iLength = cleartext.length() % 8 ? (cleartext.length() / 8 + 1) * 8 : cleartext.length();
		unsigned char* tmp = new unsigned char[iLength + 16];
		memset(tmp, 0, iLength);

		DES_ncbc_encrypt((const unsigned char*)cleartext.c_str(), tmp, cleartext.length() + 1, &keySchedule, &ivec, DES_ENCRYPT);

		strCipherText = (char*)tmp;

		delete[] tmp;
	}
	break;
	case CFB:
	{
		DES_cblock keyEncrypt, ivec;
		memset(keyEncrypt, 0, 8);

		if (key.length() <= 8)
			memcpy(keyEncrypt, key.c_str(), key.length());
		else
			memcpy(keyEncrypt, key.c_str(), 8);

		DES_key_schedule keySchedule;
		DES_set_key_unchecked(&keyEncrypt, &keySchedule);

		memcpy(ivec, cbc_iv, sizeof(cbc_iv));

		unsigned char* outputText = new unsigned char[cleartext.length()];
		memset(outputText, 0, cleartext.length());

		const unsigned char* tmp = (const unsigned char*)cleartext.c_str();

		DES_cfb_encrypt(tmp, outputText, 8, cleartext.length(), &keySchedule, &ivec, DES_ENCRYPT);

		strCipherText = (char*)outputText;

		delete[] outputText;
	}
	break;
	case TRIPLE_ECB:
	{
		DES_cblock ke1, ke2, ke3;
		memset(ke1, 0, 8);
		memset(ke2, 0, 8);
		memset(ke2, 0, 8);

		if (key.length() >= 24) {
			memcpy(ke1, key.c_str(), 8);
			memcpy(ke2, key.c_str() + 8, 8);
			memcpy(ke3, key.c_str() + 16, 8);
		}
		else if (key.length() >= 16) {
			memcpy(ke1, key.c_str(), 8);
			memcpy(ke2, key.c_str() + 8, 8);
			memcpy(ke3, key.c_str() + 16, key.length() - 16);
		}
		else if (key.length() >= 8) {
			memcpy(ke1, key.c_str(), 8);
			memcpy(ke2, key.c_str() + 8, key.length() - 8);
			memcpy(ke3, key.c_str(), 8);
		}
		else {
			memcpy(ke1, key.c_str(), key.length());
			memcpy(ke2, key.c_str(), key.length());
			memcpy(ke3, key.c_str(), key.length());
		}

		DES_key_schedule ks1, ks2, ks3;
		DES_set_key_unchecked(&ke1, &ks1);
		DES_set_key_unchecked(&ke2, &ks2);
		DES_set_key_unchecked(&ke3, &ks3);

		const_DES_cblock inputText;
		DES_cblock outputText;
		std::vector<unsigned char> vecCiphertext;
		unsigned char tmp[8];

		for (int i = 0; i < cleartext.length() / 8; i++) {
			memcpy(inputText, cleartext.c_str() + i * 8, 8);
			DES_ecb3_encrypt(&inputText, &outputText, &ks1, &ks2, &ks3, DES_ENCRYPT);
			memcpy(tmp, outputText, 8);

			for (int j = 0; j < 8; j++)
				vecCiphertext.push_back(tmp[j]);
		}

		if (cleartext.length() % 8 != 0) {
			int tmp1 = cleartext.length() / 8 * 8;
			int tmp2 = cleartext.length() - tmp1;
			memset(inputText, 0, 8);
			memcpy(inputText, cleartext.c_str() + tmp1, tmp2);

			DES_ecb3_encrypt(&inputText, &outputText, &ks1, &ks2, &ks3, DES_ENCRYPT);
			memcpy(tmp, outputText, 8);

			for (int j = 0; j < 8; j++)
				vecCiphertext.push_back(tmp[j]);
		}

		strCipherText.clear();
		strCipherText.assign(vecCiphertext.begin(), vecCiphertext.end());
	}
	break;
	case TRIPLE_CBC:
	{
		DES_cblock ke1, ke2, ke3, ivec;
		memset(ke1, 0, 8);
		memset(ke2, 0, 8);
		memset(ke2, 0, 8);

		if (key.length() >= 24) {
			memcpy(ke1, key.c_str(), 8);
			memcpy(ke2, key.c_str() + 8, 8);
			memcpy(ke3, key.c_str() + 16, 8);
		}
		else if (key.length() >= 16) {
			memcpy(ke1, key.c_str(), 8);
			memcpy(ke2, key.c_str() + 8, 8);
			memcpy(ke3, key.c_str() + 16, key.length() - 16);
		}
		else if (key.length() >= 8) {
			memcpy(ke1, key.c_str(), 8);
			memcpy(ke2, key.c_str() + 8, key.length() - 8);
			memcpy(ke3, key.c_str(), 8);
		}
		else {
			memcpy(ke1, key.c_str(), key.length());
			memcpy(ke2, key.c_str(), key.length());
			memcpy(ke3, key.c_str(), key.length());
		}

		DES_key_schedule ks1, ks2, ks3;
		DES_set_key_unchecked(&ke1, &ks1);
		DES_set_key_unchecked(&ke2, &ks2);
		DES_set_key_unchecked(&ke3, &ks3);

		memcpy(ivec, cbc_iv, sizeof(cbc_iv));

		int iLength = cleartext.length() % 8 ? (cleartext.length() / 8 + 1) * 8 : cleartext.length();
		unsigned char* tmp = new unsigned char[iLength + 16];
		memset(tmp, 0, iLength);

		DES_ede3_cbc_encrypt((const unsigned char*)cleartext.c_str(), tmp, cleartext.length() + 1, &ks1, &ks2, &ks3, &ivec, DES_ENCRYPT);

		strCipherText = (char*)tmp;

		delete[] tmp;
	}
	break;
	}
	strCipherText = HexToStr((PBYTE)strCipherText.c_str(), strCipherText.length());
	return strCipherText;
}

std::string CCEncryption::DES_Decrypt(const std::string ciphertext, const std::string key, CRYPTO_MODE mode)
{
	std::string strClearText;
	std::string ciphertextEx = StrToHex(ciphertext);
	switch (mode) {
	case GENERAL:
	case ECB:
	{
		DES_cblock keyEncrypt;
		memset(keyEncrypt, 0, 8);
		if (key.length() <= 8)
			memcpy(keyEncrypt, key.c_str(), key.length());
		else
			memcpy(keyEncrypt, key.c_str(), 8);
		DES_key_schedule keySchedule;
		DES_set_key_unchecked(&keyEncrypt, &keySchedule);
		const_DES_cblock inputText;
		DES_cblock outputText;
		std::vector<unsigned char> vecCleartext;
		unsigned char tmp[8];
		for (int i = 0; i < ciphertextEx.length() / 8; i++) {
			memcpy(inputText, ciphertextEx.c_str() + i * 8, 8);
			DES_ecb_encrypt(&inputText, &outputText, &keySchedule, DES_DECRYPT);
			memcpy(tmp, outputText, 8);

			for (int j = 0; j < 8; j++)
				vecCleartext.push_back(tmp[j]);
		}
		if (ciphertextEx.length() % 8 != 0) {
			int tmp1 = ciphertextEx.length() / 8 * 8;
			int tmp2 = ciphertextEx.length() - tmp1;
			memset(inputText, 0, 8);
			memcpy(inputText, ciphertextEx.c_str() + tmp1, tmp2);

			DES_ecb_encrypt(&inputText, &outputText, &keySchedule, DES_DECRYPT);
			memcpy(tmp, outputText, 8);

			for (int j = 0; j < 8; j++)
				vecCleartext.push_back(tmp[j]);
		}
		strClearText.clear();
		strClearText.assign(vecCleartext.begin(), vecCleartext.end());
	}
	break;
	case CBC:
	{
		DES_cblock keyEncrypt, ivec;
		memset(keyEncrypt, 0, 8);

		if (key.length() <= 8)
			memcpy(keyEncrypt, key.c_str(), key.length());
		else
			memcpy(keyEncrypt, key.c_str(), 8);

		DES_key_schedule keySchedule;
		DES_set_key_unchecked(&keyEncrypt, &keySchedule);

		memcpy(ivec, cbc_iv, sizeof(cbc_iv));

		int iLength = ciphertextEx.length() % 8 ? (ciphertextEx.length() / 8 + 1) * 8 : ciphertextEx.length();
		unsigned char* tmp = new unsigned char[iLength];
		memset(tmp, 0, iLength);

		DES_ncbc_encrypt((const unsigned char*)ciphertextEx.c_str(), tmp, ciphertextEx.length(), &keySchedule, &ivec, DES_DECRYPT);

		strClearText = (char*)tmp;

		delete[] tmp;
	}
	break;
	case CFB:
	{
		DES_cblock keyEncrypt, ivec;
		memset(keyEncrypt, 0, 8);

		if (key.length() <= 8)
			memcpy(keyEncrypt, key.c_str(), key.length());
		else
			memcpy(keyEncrypt, key.c_str(), 8);

		DES_key_schedule keySchedule;
		DES_set_key_unchecked(&keyEncrypt, &keySchedule);

		memcpy(ivec, cbc_iv, sizeof(cbc_iv));

		unsigned char* outputText = new unsigned char[ciphertextEx.length()];
		memset(outputText, 0, ciphertextEx.length());

		const unsigned char* tmp = (const unsigned char*)ciphertextEx.c_str();

		DES_cfb_encrypt(tmp, outputText, 8, 32/*ciphertextEx.length() - 16*/, &keySchedule, &ivec, DES_DECRYPT);

		strClearText = (char*)outputText;

		delete[] outputText;
	}
	break;
	case TRIPLE_ECB:
	{
		DES_cblock ke1, ke2, ke3;
		memset(ke1, 0, 8);
		memset(ke2, 0, 8);
		memset(ke2, 0, 8);

		if (key.length() >= 24) {
			memcpy(ke1, key.c_str(), 8);
			memcpy(ke2, key.c_str() + 8, 8);
			memcpy(ke3, key.c_str() + 16, 8);
		}
		else if (key.length() >= 16) {
			memcpy(ke1, key.c_str(), 8);
			memcpy(ke2, key.c_str() + 8, 8);
			memcpy(ke3, key.c_str() + 16, key.length() - 16);
		}
		else if (key.length() >= 8) {
			memcpy(ke1, key.c_str(), 8);
			memcpy(ke2, key.c_str() + 8, key.length() - 8);
			memcpy(ke3, key.c_str(), 8);
		}
		else {
			memcpy(ke1, key.c_str(), key.length());
			memcpy(ke2, key.c_str(), key.length());
			memcpy(ke3, key.c_str(), key.length());
		}

		DES_key_schedule ks1, ks2, ks3;
		DES_set_key_unchecked(&ke1, &ks1);
		DES_set_key_unchecked(&ke2, &ks2);
		DES_set_key_unchecked(&ke3, &ks3);

		const_DES_cblock inputText;
		DES_cblock outputText;
		std::vector<unsigned char> vecCleartext;
		unsigned char tmp[8];

		for (int i = 0; i < ciphertextEx.length() / 8; i++) {
			memcpy(inputText, ciphertextEx.c_str() + i * 8, 8);
			DES_ecb3_encrypt(&inputText, &outputText, &ks1, &ks2, &ks3, DES_DECRYPT);
			memcpy(tmp, outputText, 8);

			for (int j = 0; j < 8; j++)
				vecCleartext.push_back(tmp[j]);
		}

		if (ciphertextEx.length() % 8 != 0) {
			int tmp1 = ciphertextEx.length() / 8 * 8;
			int tmp2 = ciphertextEx.length() - tmp1;
			memset(inputText, 0, 8);
			memcpy(inputText, ciphertextEx.c_str() + tmp1, tmp2);

			DES_ecb3_encrypt(&inputText, &outputText, &ks1, &ks2, &ks3, DES_DECRYPT);
			memcpy(tmp, outputText, 8);

			for (int j = 0; j < 8; j++)
				vecCleartext.push_back(tmp[j]);
		}

		strClearText.clear();
		strClearText.assign(vecCleartext.begin(), vecCleartext.end());
	}
	break;
	case TRIPLE_CBC:
	{
		DES_cblock ke1, ke2, ke3, ivec;
		memset(ke1, 0, 8);
		memset(ke2, 0, 8);
		memset(ke2, 0, 8);

		if (key.length() >= 24) {
			memcpy(ke1, key.c_str(), 8);
			memcpy(ke2, key.c_str() + 8, 8);
			memcpy(ke3, key.c_str() + 16, 8);
		}
		else if (key.length() >= 16) {
			memcpy(ke1, key.c_str(), 8);
			memcpy(ke2, key.c_str() + 8, 8);
			memcpy(ke3, key.c_str() + 16, key.length() - 16);
		}
		else if (key.length() >= 8) {
			memcpy(ke1, key.c_str(), 8);
			memcpy(ke2, key.c_str() + 8, key.length() - 8);
			memcpy(ke3, key.c_str(), 8);
		}
		else {
			memcpy(ke1, key.c_str(), key.length());
			memcpy(ke2, key.c_str(), key.length());
			memcpy(ke3, key.c_str(), key.length());
		}

		DES_key_schedule ks1, ks2, ks3;
		DES_set_key_unchecked(&ke1, &ks1);
		DES_set_key_unchecked(&ke2, &ks2);
		DES_set_key_unchecked(&ke3, &ks3);

		memcpy(ivec, cbc_iv, sizeof(cbc_iv));

		int iLength = ciphertextEx.length() % 8 ? (ciphertextEx.length() / 8 + 1) * 8 : ciphertextEx.length();
		unsigned char* tmp = new unsigned char[iLength];
		memset(tmp, 0, iLength);

		DES_ede3_cbc_encrypt((const unsigned char*)ciphertextEx.c_str(), tmp, ciphertextEx.length() + 1, &ks1, &ks2, &ks3, &ivec, DES_DECRYPT);

		strClearText = (char*)tmp;

		delete[] tmp;
	}
	break;
	}

	return strClearText;
}
std::string CCEncryption::Gbk_To_Utf8(const char* szBuff)
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
std::string CCEncryption::AES_Decrypt(const std::string strKey, const std::string strData)
{
	AES_KEY key;
	std::string strRes;
	BYTE iv[] = "0102030405060708";
	int nKeyRes = -1;
	nKeyRes = AES_set_decrypt_key((PBYTE)strKey.c_str(), strKey.length() * 8, &key);
	if (NULL != nKeyRes)
		return strRes;
	std::string m_strData = aip::base64_decode(strData.c_str());
	size_t  nUlens = m_strData.length();
	for (size_t i = 0; i < nUlens / AES_BLOCK_SIZE; i++)
	{
		std::string str16 = m_strData.substr(i*AES_BLOCK_SIZE, AES_BLOCK_SIZE);
		BYTE out[AES_BLOCK_SIZE];
		ZeroMemory(out, AES_BLOCK_SIZE);
		AES_cbc_encrypt((PBYTE)str16.c_str(), (PBYTE)out, AES_BLOCK_SIZE,&key,iv, AES_DECRYPT);
		strRes += std::string((PCSTR)out, AES_BLOCK_SIZE);
	}
	return strRes;
}
std::string CCEncryption::AES_Encrypt(const std::string strKey, const std::string strData)
{
	std::string strRes;
	AES_KEY key;
	BYTE iv[] = "0102030405060708";
	int nKeyRes = -1;
	nKeyRes = AES_set_encrypt_key((PBYTE)strKey.c_str(), strKey.length() * 8, &key);
	if (NULL != nKeyRes)
		return strRes;
	std::string m_strData = strData;
	size_t  uNlens = m_strData.length();
	int nPadding = 0;	//填充的数量
	if (uNlens % AES_BLOCK_SIZE > 0)
		nPadding = AES_BLOCK_SIZE - uNlens % AES_BLOCK_SIZE;
	uNlens += nPadding;
	for (size_t i = 0; i < nPadding; i++)
		m_strData += (char)0x0;
	for (size_t l = 0; l < uNlens / AES_BLOCK_SIZE; l++)
	{
		std::string strU16 = m_strData.substr(l*AES_BLOCK_SIZE, AES_BLOCK_SIZE);
		BYTE out[AES_BLOCK_SIZE];
		ZeroMemory(out, AES_BLOCK_SIZE);
		AES_cbc_encrypt((PBYTE)strU16.c_str(),(PBYTE) out, AES_BLOCK_SIZE, &key,iv, AES_ENCRYPT);
		strRes += std::string((PCSTR)out, AES_BLOCK_SIZE);
		//strRes = out;
	}
	strRes = aip::base64_encode(strRes.c_str(), strRes.length());
	return strRes;
}


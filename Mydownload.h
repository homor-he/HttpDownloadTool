#ifndef __MYDOWNLOAD_H__
#define __MYDOWNLOAD_H__

#if _MSC_VER > 1000
#pragma once
#endif // !_MSC_VER > 1000

#include "ServiceDefine.h"
//#include "BSBuffer.h"
#include <wininet.h>
#pragma comment(lib, "Wininet.lib") 
#pragma comment(lib, "ws2_32.lib")


#define MAX_RECV_LEN           100   // 每次接收最大字符串长度.
#define MAX_PENDING_CONNECTS   4     // 等待队列的长度.

//断点续传时使用该结构体
class  CHttpSect
{
	// 例："http://a3.att.hudong.com/14/75/01300000164186121366756803686.jpg"
	// szHostAddr="a3.att.hudong.com",
	// szHttpAddr="http://a3.att.hudong.com/14/75/",
	// szHttpFilename="01300000164186121366756803686.jpg".
public:
	string   szProxyAddr;     // 代理服务器地址.
	string   szHostAddr;      // Host地址.
	int      nProxyPort;      // 代理服务端口号.
	int      nHostPort;       // Host端口号.
	string   szHttpAddr;      // Http文件地址.
	string   szHttpFilename;  // Http文件名.
	string   szDesFilename;   // 下载后的文件名. 
	DWORD    nStart;          // 分割的起始位置.
	DWORD    nEnd;            // 分割的起始位置.
	DWORD    bProxyMode;      // 下载模态.
	DWORD    callerID;		  // 调用者线程
};

class  CHttpGet
{
public:
	CHttpGet();
	virtual ~CHttpGet();
	//static unsigned long m_downloaded;

private:
	CHttpSect *sectinfo;
	static int m_nCount;
	static UINT ThreadDownLoad(void* pParam);
public:
	static long m_nFileLength;
private:
	static SOCKET ConnectHttpProxy(string strProxyAddr, int nPort);
	static SOCKET ConnectHttpNonProxy(string strHostAddr, int nPort);
	static BOOL SendHttpHeader(SOCKET hSocket, string strHostAddr,
		string strHttpAddr, string strHttpFilename, DWORD nPos);
	static DWORD GetHttpHeader(SOCKET sckDest, char *str);
	static long GetFileLength(char *httpHeader);
	static BOOL SocketSend(SOCKET sckDest, string szHttp);

	BOOL FileCombine(CHttpSect *pInfo, FILE *fpwrite);
public:
	//http请求并将数据放置在程序缓存中，常用于请求json、xml等格式的数据，可直接调用
	//static BOOL HttpReqGetDataInBuff(char* urlPath, CBSBuffer& buffer);

	//http请求并将数据存放于本地，常用于下载不适用于断点续传的文件，可直接调用
	//urlPath:网上资源路径,例："http://a3.att.hudong.com/14/75/01300000164186121366756803686.jpg"；
	//localPath:本地存储路径,例："D:\\BSServer\\DownLoadFile\\Texture\\";
	//fileName:存储文件名，例："tex.jpg";
	//static BOOL HttpReqDownloadFile(char * urlPath, char * localPath, char * fileName = NULL);

	BOOL HttpDownLoadProxy(
		string & strProxyAddr,
		int nProxyPort,
		string & strHostAddr,
		string & strHttpAddr,
		string & strHttpFilename,
		string & strWriteFileName,
		int nSectNum,
		DWORD &totalSize,
		DWORD &callerID
	);

	BOOL HttpDownLoadNonProxy(
		string & urlPath,
		string & strHostAddr,
		string & strHttpAddr,
		string & strHttpFilename,
		string & strWriteFileName,
		int nSectNum,
		DWORD &totalSize,
		DWORD &callerID
	);

	BOOL HttpDownLoad(
		string strProxyAddr,
		int nProxyPort,
		string & strHostAddr,
		int nHostPort,
		string & strHttpAddr,
		string & strHttpFilename,
		string & strWriteFileName,
		int nSectNum,
		BOOL bProxy,
		DWORD &callerID
	);
};


class CDealSocket
{
public:
	CDealSocket();
	virtual ~CDealSocket();
public:
	SOCKET GetConnect(string host, int port);
	SOCKET Listening(int port);
	string GetResponse(SOCKET hSock);
};

class CMyFile
{
public:
	CMyFile();
	virtual ~CMyFile();
public:
	BOOL FileExists(LPCTSTR lpszFileName);
	FILE* GetFilePointer(LPCTSTR lpszFileName);
	DWORD GetFileSizeByName(LPCTSTR lpszFileName);
	string GetShortFileName(LPCSTR lpszFullPathName);
};

extern CMyFile myfile;
extern CDealSocket dealsocket;
extern unsigned long rdownloaded;

#endif // !__MYDOWNLOAD_H__

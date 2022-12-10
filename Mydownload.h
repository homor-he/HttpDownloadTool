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


#define MAX_RECV_LEN           100   // ÿ�ν�������ַ�������.
#define MAX_PENDING_CONNECTS   4     // �ȴ����еĳ���.

//�ϵ�����ʱʹ�øýṹ��
class  CHttpSect
{
	// ����"http://a3.att.hudong.com/14/75/01300000164186121366756803686.jpg"
	// szHostAddr="a3.att.hudong.com",
	// szHttpAddr="http://a3.att.hudong.com/14/75/",
	// szHttpFilename="01300000164186121366756803686.jpg".
public:
	string   szProxyAddr;     // �����������ַ.
	string   szHostAddr;      // Host��ַ.
	int      nProxyPort;      // �������˿ں�.
	int      nHostPort;       // Host�˿ں�.
	string   szHttpAddr;      // Http�ļ���ַ.
	string   szHttpFilename;  // Http�ļ���.
	string   szDesFilename;   // ���غ���ļ���. 
	DWORD    nStart;          // �ָ����ʼλ��.
	DWORD    nEnd;            // �ָ����ʼλ��.
	DWORD    bProxyMode;      // ����ģ̬.
	DWORD    callerID;		  // �������߳�
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
	//http���󲢽����ݷ����ڳ��򻺴��У�����������json��xml�ȸ�ʽ�����ݣ���ֱ�ӵ���
	//static BOOL HttpReqGetDataInBuff(char* urlPath, CBSBuffer& buffer);

	//http���󲢽����ݴ���ڱ��أ����������ز������ڶϵ��������ļ�����ֱ�ӵ���
	//urlPath:������Դ·��,����"http://a3.att.hudong.com/14/75/01300000164186121366756803686.jpg"��
	//localPath:���ش洢·��,����"D:\\BSServer\\DownLoadFile\\Texture\\";
	//fileName:�洢�ļ���������"tex.jpg";
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

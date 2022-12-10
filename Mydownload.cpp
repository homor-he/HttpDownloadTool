#include "MyDownload.h"
#include "ChineseCode.h"
#include "DownloadHelpMgr.h"
#include <thread>
#include <iostream>

int CHttpGet::m_nCount;
long CHttpGet::m_nFileLength = 0;
//unsigned long CHttpGet::m_downloaded = 0;
unsigned long rdownloaded = 0;

//---------------------------------------------------------------------------
CHttpGet::CHttpGet()
{
	m_nFileLength = 0;
}

//---------------------------------------------------------------------------
CHttpGet::~CHttpGet()
{
}

//---------------------------------------------------------------------------
BOOL CHttpGet::HttpDownLoadProxy(
	string & strProxyAddr,
	int nProxyPort,
	string & strHostAddr,
	string & strHttpAddr,
	string & strHttpFilename,
	string & strWriteFileName,
	int nSectNum,
	DWORD &totalSize,
	DWORD &callerID
)
{
	SOCKET hSocket;
	hSocket = ConnectHttpProxy(strProxyAddr, nProxyPort);
	if (hSocket == INVALID_SOCKET) 
		return TRUE;
	// �����ļ�ͷ�������ļ���С.
	SendHttpHeader(hSocket, strHostAddr, strHttpAddr, strHttpFilename, 0);
	closesocket(hSocket);

	if (CHttpGet::m_nFileLength >= 0)
	{
		totalSize = CHttpGet::m_nFileLength;
		if (!HttpDownLoad(strProxyAddr, nProxyPort, strHostAddr, 80, strHttpAddr, strHttpFilename, strWriteFileName, nSectNum, true,callerID))
			return FALSE;
	}
	return TRUE;
}

//---------------------------------------------------------------------------
BOOL CHttpGet::HttpDownLoadNonProxy(
	string & urlPath,
	string & strHostAddr,
	string & strHttpAddr,
	string & strHttpFilename,
	string & strWriteFileName,
	int nSectNum,
	DWORD &totalSize,
	DWORD &callerID
)
{
	int nHostPort = 80;
	SOCKET hSocket;
	hSocket = ConnectHttpNonProxy(strHostAddr, nHostPort);
	if (hSocket == INVALID_SOCKET) 
		return TRUE;
	// �����ļ�ͷ�������ļ���С.
	SendHttpHeader(hSocket, strHostAddr, strHttpAddr, strHttpFilename, 0);
	closesocket(hSocket);
	
	//С��0˵�����ļ�û���ܳ��ȴ�С���޷����ϵ������ķ�������
	if (CHttpGet::m_nFileLength < 0)
	{
		/*int nPos = strWriteFileName.find(strHttpFilename);
		string path = strWriteFileName.substr(0, nPos);
		HttpReqDownloadFile((char*)urlPath.c_str(), (char*)path.c_str(), NULL);*/
	}
	else
	{
		totalSize = CHttpGet::m_nFileLength;
		if (!HttpDownLoad("", 80, strHostAddr, nHostPort, strHttpAddr, strHttpFilename, 
			strWriteFileName, nSectNum, false,callerID))
			return FALSE;
	}
	return TRUE;
}

//---------------------------------------------------------------------------
BOOL CHttpGet::HttpDownLoad(
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
)
{
	m_nCount = 0;											 // �Ѽ���������.
	sectinfo = new CHttpSect[nSectNum];						 // ����Ϣ�ṹ�����ڴ�.
	DWORD nSize = m_nFileLength / nSectNum;					 // ����ָ�εĴ�С.

	int i;
	// �����߳�.���ܳ���50��
	thread* pThread[50];
	for (i = 0; i<nSectNum; i++)
	{
		sectinfo[i].szProxyAddr = strProxyAddr;				 // �����������ַ.
		sectinfo[i].nProxyPort = nProxyPort;			     // Host��ַ.
		sectinfo[i].szHostAddr = strHostAddr;				 // Http�ļ���ַ.
		sectinfo[i].nHostPort = nHostPort;					 // Http�˿�.
		sectinfo[i].szHttpAddr = strHttpAddr;				 // ��Դ�ļ�·��.
		sectinfo[i].szHttpFilename = strHttpFilename;		 // ��Դ���ļ���.
		sectinfo[i].bProxyMode = bProxy;					 // ����ģ̬. 
		sectinfo[i].callerID = callerID;

		// ������ʱ�ļ���.
		string strTempFileName;
		char tempfileName[1024] = { 0 };
		sprintf_s(tempfileName, "%s_%d", strWriteFileName.c_str(), i);
		strTempFileName = string(tempfileName);
		sectinfo[i].szDesFilename = strTempFileName;		 // ���غ���ļ���.

		if (i<nSectNum - 1) 
		{
			sectinfo[i].nStart = i*nSize;					 
			sectinfo[i].nEnd = (i + 1)*nSize;				 
		}
		else
		{
			sectinfo[i].nStart = i*nSize;				     						
			sectinfo[i].nEnd = m_nFileLength;      
		}

		pThread[i] = new thread(ThreadDownLoad, &sectinfo[i]);
	}
	// �ȴ������߳̽���.
	//WaitForMultipleObjects(nSectNum, hThread, TRUE, INFINITE);
	for (int ii = 0; ii < nSectNum; ii++)
	{
		pThread[ii]->join();
	}
	for (int j = 0; j < nSectNum; ++j)
	{
		delete pThread[j];
	}

	//��ĳ���ļ���δ�����꣬���ش���Ӧ�����ϲ�������������ء�
	if (m_nCount != nSectNum)
	{
		return FALSE;
	}
		
	FILE *fpwrite;
	// ��д�ļ�.
	if ((fpwrite = fopen(strWriteFileName.c_str(), "wb")) == NULL) 
	{
		return FALSE;
	}
	//�Ѽ����ֶε���ʱ�ļ�ƴ�ӳ�һ�����ļ�
	for (i = 0; i<nSectNum; i++) 
	{
		FileCombine(&sectinfo[i], fpwrite);
	}
	fclose(fpwrite);
	delete[] sectinfo;
	return TRUE;
}

//---------------------------------------------------------------------------
BOOL CHttpGet::FileCombine(CHttpSect *pInfo, FILE *fpwrite)
{
	FILE *fpread;
	// ���ļ�.
	if ((fpread = fopen(pInfo->szDesFilename.c_str(), "rb")) == NULL)
		return FALSE;
	DWORD nPos = pInfo->nStart;
	// �����ļ�дָ����ʼλ��.
	fseek(fpwrite, nPos, SEEK_SET);
	int c;
	// ���ļ�����д�뵽���ļ�.		
	while ((c = fgetc(fpread)) != EOF)
	{
		fputc(c, fpwrite);
		nPos++;
		if (nPos == pInfo->nEnd) break;
	}
	fclose(fpread);
	DeleteFile(pInfo->szDesFilename.c_str());
	return TRUE;
}

//---------------------------------------------------------------------------
UINT CHttpGet::ThreadDownLoad(void* pParam)
{
	CHttpSect *pInfo = (CHttpSect*)pParam;
	SOCKET hSocket;

	if (pInfo->bProxyMode) 
		hSocket = ConnectHttpProxy(pInfo->szProxyAddr, pInfo->nProxyPort);
	else 
		hSocket = ConnectHttpNonProxy(pInfo->szHostAddr, pInfo->nHostPort);

	if (hSocket == INVALID_SOCKET) 
		return 1;

	// ������ʱ�ļ���С��Ϊ�˶ϵ�����
	DWORD nFileSize = myfile.GetFileSizeByName(pInfo->szDesFilename.c_str());
	DWORD nSectSize = (pInfo->nEnd) - (pInfo->nStart);

	// �˶����������.
	if (nFileSize >= nSectSize) 
	{
		//BSWriteLog("%s,�ļ����������!", pInfo->szDesFilename.c_str());
		// ����
		CHttpGet::m_nCount++;  
		return 0;
	}

	FILE *fpwrite = myfile.GetFilePointer(pInfo->szDesFilename.c_str());
	if (!fpwrite) 
		return 1;

	// �������ط�Χ
	SendHttpHeader(hSocket, pInfo->szHostAddr, pInfo->szHttpAddr,
		pInfo->szHttpFilename, pInfo->nStart + nFileSize);

	// �����ļ�дָ����ʼλ�ã��ϵ�����
	fseek(fpwrite, nFileSize, SEEK_SET);

	DWORD nLen;
	DWORD nSumLen = 0;
	char szBuffer[1024];
	memset(szBuffer, 0, 1024);

	while (true)
	{
		if (nSumLen >= nSectSize - nFileSize)
			break;
		nLen = recv(hSocket, szBuffer, sizeof(szBuffer), 0);

		//ԭ�Ӳ���������ͬ����
		if (pInfo->callerID == 0)
		{
			rdownloaded += nLen;
		}
		else
		{
			gbs_lsDownloded[pInfo->callerID - 1] += nLen;
		}
		
		if (nLen == SOCKET_ERROR) 
		{
			//BSWriteLog("Read error!");
			fclose(fpwrite);
			return 1;
		}
		if (nLen == 0) 
			break;
		nSumLen += nLen;
		//BSWriteLog("%d\n", nLen);
		// ������д���ļ�.		
		fwrite(szBuffer, sizeof(char), nLen, fpwrite);
	}

	fclose(fpwrite);      // �ر�д�ļ�.
	closesocket(hSocket); // �ر��׽���.
	CHttpGet::m_nCount++; // ����.
	return 0;
}

//BOOL CHttpGet::HttpReqGetDataInBuff(char * urlPath, CBSBuffer & buffer)
//{
//	HINTERNET session = InternetOpen("GetBufferData", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
//	if (NULL != session)
//	{
//		HINTERNET hHttp = InternetOpenUrl(session, urlPath, NULL, 0, INTERNET_FLAG_DONT_CACHE, 0);
//
//		if (NULL != hHttp)
//		{
//			BSWriteLog("Http:%s, request success!", urlPath);
//			char temp[1025];
//			DWORD readed = 1;
//			while (readed)
//			{
//				memset(temp, 0, 1024);
//				InternetReadFile(hHttp, temp, 1024, &readed);
//				buffer.Append(temp, readed);
//			}
//			char a = '\0';
//			buffer.Append(&a, 1);
//			InternetCloseHandle(hHttp);
//			hHttp = NULL;
//		}
//		else
//		{
//			BSWriteLog("Http:%s, request fail!", urlPath);
//			return FALSE;
//		}
//		InternetCloseHandle(session);
//		session = NULL;
//	}
//	return TRUE;
//}

//BOOL CHttpGet::HttpReqDownloadFile(char * urlPath, char * localPath, char * fileName)
//{
//	char fullPath[1024] = { 0 };
//	if (NULL != fileName)
//	{
//		sprintf_s(fullPath, "%s%s", localPath, fileName);
//	}
//	else
//	{
//		sprintf_s(fullPath, "%s%s", localPath, myfile.GetShortFileName(urlPath).c_str());
//	}
//
//	HINTERNET session = InternetOpen("DownloadFile", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
//	if (NULL != session)
//	{
//		HINTERNET hHttp = InternetOpenUrl(session, urlPath, NULL, 0, INTERNET_FLAG_DONT_CACHE, 0);
//		if (NULL != hHttp)
//		{
//			BSWriteLog("Http:%s, request success!", urlPath);
//			char temp[1025];
//			DWORD readed = 1;
//			FILE * stream = fopen(fullPath, "ab+");
//			if (NULL != stream)
//			{
//				while (readed)
//				{
//					memset(temp, 0, 1024);
//					InternetReadFile(hHttp, temp, 1024, &readed);
//					fwrite(temp, sizeof(char), readed, stream);
//				}
//				fclose(stream);
//			}
//			InternetCloseHandle(hHttp);
//			hHttp = NULL;
//		}
//		else
//		{
//			BSWriteLog("Http:%s, request fail!", urlPath);
//			return FALSE;
//		}
//		InternetCloseHandle(session);
//		session = NULL;
//	}
//	return TRUE;
//}

//---------------------------------------------------------------------------
SOCKET CHttpGet::ConnectHttpProxy(string strProxyAddr, int nPort)
{
	//BSWriteLog("���ڽ�������");

	char sTemp[1024] = {0};
	string strTemp = "";
	char cTmpBuffer[1024];
	SOCKET hSocket = dealsocket.GetConnect(strProxyAddr, nPort);

	if (hSocket == INVALID_SOCKET)
	{
		//BSWriteLog("����http������ʧ�ܣ�");
		return INVALID_SOCKET;
	}

	// ����CONNCET�������������������ںʹ��������Ӵ����������
	// ��ַ�Ͷ˿ڷ���strProxyAddr,nPort ����.
	sprintf_s(sTemp, "CONNECT %s:%d HTTP/1.1\r\nUser-Agent:\
		           MyApp/0.1\r\n\r\n", strProxyAddr.c_str(), nPort);

	if (!SocketSend(hSocket, string(sTemp)))
	{
		//BSWriteLog("���Ӵ���ʧ��");
		return INVALID_SOCKET;
	}

	// ȡ�ô�����Ӧ��������Ӵ���ɹ��������������
	// ����"200 Connection established".
	int nLen = GetHttpHeader(hSocket, cTmpBuffer);
	memcpy(sTemp, cTmpBuffer, 1024);
	strTemp = string(sTemp);
	//sTemp = cTmpBuffer;
	if (strTemp.find("HTTP/1.0 200 Connection established", 0) == -1)
	{
		//BSWriteLog("���Ӵ���ʧ��");
		return INVALID_SOCKET;
	}

	//BSWriteLog("%s",sTemp);
	//BSWriteLog("�����������");
	return hSocket;
}

//---------------------------------------------------------------------------
SOCKET CHttpGet::ConnectHttpNonProxy(string strHostAddr, int nPort)
{
	//BSWriteLog("���ڽ�������");
	SOCKET hSocket = dealsocket.GetConnect(strHostAddr, nPort);
	if (hSocket == INVALID_SOCKET)
		return INVALID_SOCKET;

	return hSocket;
}

//---------------------------------------------------------------------------
// ����: strHostAddr="www.aitenshi.com",
// strHttpAddr="http://www.aitenshi.com/bbs/images/",
// strHttpFilename="pics.jpg".
BOOL CHttpGet::SendHttpHeader(SOCKET hSocket, string strHostAddr,
	string strHttpAddr, string strHttpFilename, DWORD nPos)
{
	// ��������. 
	string sTemp;
	char tempStr[1024] = { 0 };
	char cTmpBuffer[1024] = { 0 };
	
	// Line1: �����·��,�汾.
	sprintf_s(tempStr, "GET %s%s HTTP/1.1\r\n", strHttpAddr.c_str(), strHttpFilename.c_str());
	sTemp = string(tempStr);

	// Line2:����.
	memset(tempStr,0,1024);
	sprintf_s(tempStr, "Host: %s\r\n", strHostAddr.c_str());
	sTemp.append(tempStr);

	// Line3:���յ���������.
	memset(tempStr, 0, 1024);
	sprintf_s(tempStr, "Accept: \r\n");
	sTemp.append(tempStr);

	// Line4:�ο���ַ.
	memset(tempStr, 0, 1024);
	sprintf_s(tempStr, "Referer: %s\r\n", strHttpAddr.c_str());
	sTemp.append(tempStr);

	// Line5:���������.
	memset(tempStr, 0, 1024);
	sprintf_s(tempStr, "User-Agent: Mozilla/4.0 \
		(compatible; MSIE 5.0; Windows NT; DigExt; DTS Agent;)\r\n");
	sTemp.append(tempStr);

	// ����. Range ��Ҫ���ص����ݷ�Χ������������Ҫ.
	memset(tempStr, 0, 1024);
	sprintf_s(tempStr, "Range: bytes=%d-\r\n", nPos);
	sTemp.append(tempStr);

	// LastLine: ����.
	memset(tempStr, 0, 1024);
	sprintf_s(tempStr, "\r\n");
	sTemp.append(tempStr);
	if (!SocketSend(hSocket, sTemp)) return FALSE;

	// ȡ��httpͷ.
	int i = GetHttpHeader(hSocket, cTmpBuffer);
	if (!i)
	{
		//BSWriteLog("��ȡHTTPͷ����!");
		return 0;
	}

	// ���ȡ�õ�httpͷ����404�����������ʾ���ӳ�����.
	sTemp = cTmpBuffer;
	size_t sPos = sTemp.find_first_of("\r\n");
	if (sPos == string::npos)
		return FALSE;
	sTemp = sTemp.substr(0, sPos);
	if (sTemp.find("404") != string::npos) 
		return FALSE;

	// �õ��������ļ��Ĵ�С.
	m_nFileLength = GetFileLength(cTmpBuffer);

	//��ӡreponse header
	//BSWriteLog("%s",cTmpBuffer);
	return TRUE;
}

//---------------------------------------------------------------------------
DWORD CHttpGet::GetHttpHeader(SOCKET sckDest, char *str)
{
	BOOL bResponsed = FALSE;
	DWORD nResponseHeaderSize;

	if (!bResponsed)
	{
		char c = 0;
		int nIndex = 0;
		BOOL bEndResponse = FALSE;
		while (!bEndResponse && nIndex < 1024)
		{
			recv(sckDest, &c, 1, 0);
			str[nIndex++] = c;
			if (nIndex >= 4)
			{
				if (str[nIndex - 4] == '\r' &&
					str[nIndex - 3] == '\n' &&
					str[nIndex - 2] == '\r' &&
					str[nIndex - 1] == '\n')
					bEndResponse = TRUE;
			}
		}

		str[nIndex] = 0;
		nResponseHeaderSize = nIndex;
		bResponsed = TRUE;
	}

	return nResponseHeaderSize;
}

//---------------------------------------------------------------------------
long CHttpGet::GetFileLength(char *httpHeader)
{
	string strHeader;
	string strFind ="Content-Length:";
	int local;
	strHeader = string(httpHeader);
	local = strHeader.find(strFind, 0);
	if (local < 0)
	{
		//BSWriteLog("reponse header no Content-Length!");
		return -1;
	}
	local += strFind.size();
	strHeader = strHeader.substr(local + 1, strHeader.size() - 1);
	local = strHeader.find("\r\n");

	if (local != -1) {
		strHeader = strHeader.substr(0, local);
	}

	return atoi(strHeader.c_str());
}

//---------------------------------------------------------------------------
BOOL CHttpGet::SocketSend(SOCKET sckDest, string szHttp)
{
	int iLen = szHttp.size();
	if (send(sckDest, szHttp.c_str(), iLen, 0) == SOCKET_ERROR)
	{
		closesocket(sckDest);
		//BSWriteLog("��������ʧ��!");
		return FALSE;
	}
	return TRUE;
}


CDealSocket dealsocket;

//---------------------------------------------------------------------------
CDealSocket::CDealSocket()
{
	// �׽��ֳ�ʼ��.
	WORD wVersionRequested = MAKEWORD(1, 1);
	WSADATA wsaData;

	// ��ʼ��WinSock.
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		//BSWriteLog("WSAStartup");
		return;
	}

	// ��� WinSock �汾.
	if (wsaData.wVersion != wVersionRequested)
	{
		//BSWriteLog("WinSock version not supported");
		WSACleanup();
		return;
	}
}

//---------------------------------------------------------------------------
CDealSocket::~CDealSocket()
{
	// �ͷ�WinSock.
	WSACleanup();
}

//---------------------------------------------------------------------------
string CDealSocket::GetResponse(SOCKET hSock)
{
	char szBufferA[MAX_RECV_LEN];  	// ASCII�ַ���. 
	int	iReturn;					// recv�������ص�ֵ.

	char szError[1024] = {0};
	string strPlus = "";

	while (1)
	{
		// ���׽��ֽ�������.
		iReturn = recv(hSock, szBufferA, MAX_RECV_LEN, 0);
		szBufferA[iReturn] = 0;
		strPlus += szBufferA;

		//BSWriteLog(szBufferA);

		if (iReturn == SOCKET_ERROR)
		{
			sprintf_s(szError, "No data is received, recv failed. Error: %d",
				WSAGetLastError());
			//MessageBox(NULL, szError, TEXT("Client"), MB_OK);
			//BSWriteLog("%s", szError);
			break;
		}
		else if (iReturn<MAX_RECV_LEN) {
			//BSWriteLog("Finished receiving data");
			break;
		}
	}

	return strPlus;
}

//---------------------------------------------------------------------------
SOCKET CDealSocket::GetConnect(string host, int port)
{
	SOCKET hSocket;
	SOCKADDR_IN saServer;          // �������׽��ֵ�ַ.
	PHOSTENT phostent = NULL;	   // ָ��HOSTENT�ṹָ��.

								   // ����һ���󶨵���������TCP/IP�׽���.
	if ((hSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		//BSWriteLog("Allocating socket failed. Error: %d", WSAGetLastError());
		return INVALID_SOCKET;
	}

	// ʹ��TCP/IPЭ��.
	saServer.sin_family = AF_INET;

	// ��ȡ��������ص���Ϣ.
	if ((phostent = gethostbyname(host.c_str())) == NULL)
	{
		//BSWriteLog("Unable to get the host name. Error: %d", WSAGetLastError());
		closesocket(hSocket);
		return INVALID_SOCKET;
	}

	// ���׽���IP��ַ��ֵ.
	memcpy((char *)&(saServer.sin_addr),
		phostent->h_addr,
		phostent->h_length);

	// �趨�׽��ֶ˿ں�.
	saServer.sin_port = htons(port);

	// ���������������׽�������.
	if (connect(hSocket, (PSOCKADDR)&saServer,
		sizeof(saServer)) == SOCKET_ERROR)
	{
		//BSWriteLog("Connecting to the server failed. Error: %d", WSAGetLastError());
		closesocket(hSocket);
		return INVALID_SOCKET;
	}

	return hSocket;
}

//---------------------------------------------------------------------------
SOCKET CDealSocket::Listening(int port)
{
	SOCKET ListenSocket = INVALID_SOCKET;	// �����׽���.
	SOCKADDR_IN local_sin;				    // �����׽��ֵ�ַ.

											// ����TCP/IP�׽���.
	if ((ListenSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		//BSWriteLog("Allocating socket failed. Error: %d", WSAGetLastError());
		return INVALID_SOCKET;
	}

	// ���׽�����Ϣ�ṹ��ֵ.
	local_sin.sin_family = AF_INET;
	local_sin.sin_port = htons(port);
	local_sin.sin_addr.s_addr = htonl(INADDR_ANY);

	// ���б�����ַ������׽��ְ�.
	//bind(ListenSocket, (sockaddr *)&local_sin, sizeof(local_sin))
	if (SOCKET_ERROR == ::bind(ListenSocket,(sockaddr*)&local_sin,sizeof(local_sin)))
	{
		//BSWriteLog("Binding socket failed. Error: %d", WSAGetLastError());
		closesocket(ListenSocket);
		return INVALID_SOCKET;
	}

	// �����׽��ֶ��ⲿ���ӵļ���.
	if (listen(ListenSocket, MAX_PENDING_CONNECTS) == SOCKET_ERROR)
	{
		//BSWriteLog("Listening to the client failed. Error: %d",WSAGetLastError());
		closesocket(ListenSocket);
		return INVALID_SOCKET;
	}

	return ListenSocket;
}

CMyFile myfile;

//---------------------------------------------------------------------------
CMyFile::CMyFile()
{
}

//---------------------------------------------------------------------------
CMyFile::~CMyFile()
{
}

//---------------------------------------------------------------------------
BOOL CMyFile::FileExists(LPCTSTR lpszFileName)
{
	DWORD dwAttributes = GetFileAttributes(lpszFileName);
	if (dwAttributes == 0xFFFFFFFF)
		return FALSE;

	if ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

//---------------------------------------------------------------------------
FILE* CMyFile::GetFilePointer(LPCTSTR lpszFileName)
{
	FILE *fp = NULL;
	if (FileExists(lpszFileName)) {
		// �������ļ�����д����.
		fp = fopen(lpszFileName, "r+b");
	}
	else {
		// �������ļ�����д����.
		fp = fopen(lpszFileName, "w+b");
	}

	return fp;
}

//---------------------------------------------------------------------------
DWORD CMyFile::GetFileSizeByName(LPCTSTR lpszFileName)
{
	if (!FileExists(lpszFileName)) return 0;
	struct _stat ST;
	// ��ȡ�ļ�����.
	_stat(lpszFileName, &ST);
	UINT nFilesize = ST.st_size;
	return nFilesize;
}

//---------------------------------------------------------------------------
// ��ȫ���ļ�������ȡ���ļ���.
string CMyFile::GetShortFileName(LPCSTR lpszFullPathName)
{
	string strFileName = lpszFullPathName;
	string strShortName;
	int n = strFileName.rfind('/');
	strShortName = strFileName.substr(n + 1, strFileName.size() - 1);
	return strShortName;
}
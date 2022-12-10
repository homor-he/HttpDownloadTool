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
	// 发送文件头，计算文件大小.
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
	// 发送文件头，计算文件大小.
	SendHttpHeader(hSocket, strHostAddr, strHttpAddr, strHttpFilename, 0);
	closesocket(hSocket);
	
	//小于0说明该文件没有总长度大小，无法按断点续传的方法下载
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
	m_nCount = 0;											 // 把计数器清零.
	sectinfo = new CHttpSect[nSectNum];						 // 给信息结构申请内存.
	DWORD nSize = m_nFileLength / nSectNum;					 // 计算分割段的大小.

	int i;
	// 创建线程.不能超过50个
	thread* pThread[50];
	for (i = 0; i<nSectNum; i++)
	{
		sectinfo[i].szProxyAddr = strProxyAddr;				 // 代理服务器地址.
		sectinfo[i].nProxyPort = nProxyPort;			     // Host地址.
		sectinfo[i].szHostAddr = strHostAddr;				 // Http文件地址.
		sectinfo[i].nHostPort = nHostPort;					 // Http端口.
		sectinfo[i].szHttpAddr = strHttpAddr;				 // 资源文件路径.
		sectinfo[i].szHttpFilename = strHttpFilename;		 // 资源的文件名.
		sectinfo[i].bProxyMode = bProxy;					 // 下载模态. 
		sectinfo[i].callerID = callerID;

		// 计算临时文件名.
		string strTempFileName;
		char tempfileName[1024] = { 0 };
		sprintf_s(tempfileName, "%s_%d", strWriteFileName.c_str(), i);
		strTempFileName = string(tempfileName);
		sectinfo[i].szDesFilename = strTempFileName;		 // 下载后的文件名.

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
	// 等待所有线程结束.
	//WaitForMultipleObjects(nSectNum, hThread, TRUE, INFINITE);
	for (int ii = 0; ii < nSectNum; ii++)
	{
		pThread[ii]->join();
	}
	for (int j = 0; j < nSectNum; ++j)
	{
		delete pThread[j];
	}

	//有某个文件块未下载完，返回错误，应由最上层调用者重新下载。
	if (m_nCount != nSectNum)
	{
		return FALSE;
	}
		
	FILE *fpwrite;
	// 打开写文件.
	if ((fpwrite = fopen(strWriteFileName.c_str(), "wb")) == NULL) 
	{
		return FALSE;
	}
	//把几个分段的临时文件拼接成一个总文件
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
	// 打开文件.
	if ((fpread = fopen(pInfo->szDesFilename.c_str(), "rb")) == NULL)
		return FALSE;
	DWORD nPos = pInfo->nStart;
	// 设置文件写指针起始位置.
	fseek(fpwrite, nPos, SEEK_SET);
	int c;
	// 把文件数据写入到子文件.		
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

	// 计算临时文件大小，为了断点续传
	DWORD nFileSize = myfile.GetFileSizeByName(pInfo->szDesFilename.c_str());
	DWORD nSectSize = (pInfo->nEnd) - (pInfo->nStart);

	// 此段已下载完毕.
	if (nFileSize >= nSectSize) 
	{
		//BSWriteLog("%s,文件已下载完毕!", pInfo->szDesFilename.c_str());
		// 计数
		CHttpGet::m_nCount++;  
		return 0;
	}

	FILE *fpwrite = myfile.GetFilePointer(pInfo->szDesFilename.c_str());
	if (!fpwrite) 
		return 1;

	// 设置下载范围
	SendHttpHeader(hSocket, pInfo->szHostAddr, pInfo->szHttpAddr,
		pInfo->szHttpFilename, pInfo->nStart + nFileSize);

	// 设置文件写指针起始位置，断点续传
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

		//原子操作，不用同步。
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
		// 把数据写入文件.		
		fwrite(szBuffer, sizeof(char), nLen, fpwrite);
	}

	fclose(fpwrite);      // 关闭写文件.
	closesocket(hSocket); // 关闭套接字.
	CHttpGet::m_nCount++; // 计数.
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
	//BSWriteLog("正在建立连接");

	char sTemp[1024] = {0};
	string strTemp = "";
	char cTmpBuffer[1024];
	SOCKET hSocket = dealsocket.GetConnect(strProxyAddr, nPort);

	if (hSocket == INVALID_SOCKET)
	{
		//BSWriteLog("连接http服务器失败！");
		return INVALID_SOCKET;
	}

	// 发送CONNCET请求令到代理服务器，用于和代理建立连接代理服务器的
	// 地址和端口放在strProxyAddr,nPort 里面.
	sprintf_s(sTemp, "CONNECT %s:%d HTTP/1.1\r\nUser-Agent:\
		           MyApp/0.1\r\n\r\n", strProxyAddr.c_str(), nPort);

	if (!SocketSend(hSocket, string(sTemp)))
	{
		//BSWriteLog("连接代理失败");
		return INVALID_SOCKET;
	}

	// 取得代理响应，如果连接代理成功，代理服务器将
	// 返回"200 Connection established".
	int nLen = GetHttpHeader(hSocket, cTmpBuffer);
	memcpy(sTemp, cTmpBuffer, 1024);
	strTemp = string(sTemp);
	//sTemp = cTmpBuffer;
	if (strTemp.find("HTTP/1.0 200 Connection established", 0) == -1)
	{
		//BSWriteLog("连接代理失败");
		return INVALID_SOCKET;
	}

	//BSWriteLog("%s",sTemp);
	//BSWriteLog("代理连接完成");
	return hSocket;
}

//---------------------------------------------------------------------------
SOCKET CHttpGet::ConnectHttpNonProxy(string strHostAddr, int nPort)
{
	//BSWriteLog("正在建立连接");
	SOCKET hSocket = dealsocket.GetConnect(strHostAddr, nPort);
	if (hSocket == INVALID_SOCKET)
		return INVALID_SOCKET;

	return hSocket;
}

//---------------------------------------------------------------------------
// 例如: strHostAddr="www.aitenshi.com",
// strHttpAddr="http://www.aitenshi.com/bbs/images/",
// strHttpFilename="pics.jpg".
BOOL CHttpGet::SendHttpHeader(SOCKET hSocket, string strHostAddr,
	string strHttpAddr, string strHttpFilename, DWORD nPos)
{
	// 进行下载. 
	string sTemp;
	char tempStr[1024] = { 0 };
	char cTmpBuffer[1024] = { 0 };
	
	// Line1: 请求的路径,版本.
	sprintf_s(tempStr, "GET %s%s HTTP/1.1\r\n", strHttpAddr.c_str(), strHttpFilename.c_str());
	sTemp = string(tempStr);

	// Line2:主机.
	memset(tempStr,0,1024);
	sprintf_s(tempStr, "Host: %s\r\n", strHostAddr.c_str());
	sTemp.append(tempStr);

	// Line3:接收的数据类型.
	memset(tempStr, 0, 1024);
	sprintf_s(tempStr, "Accept: \r\n");
	sTemp.append(tempStr);

	// Line4:参考地址.
	memset(tempStr, 0, 1024);
	sprintf_s(tempStr, "Referer: %s\r\n", strHttpAddr.c_str());
	sTemp.append(tempStr);

	// Line5:浏览器类型.
	memset(tempStr, 0, 1024);
	sprintf_s(tempStr, "User-Agent: Mozilla/4.0 \
		(compatible; MSIE 5.0; Windows NT; DigExt; DTS Agent;)\r\n");
	sTemp.append(tempStr);

	// 续传. Range 是要下载的数据范围，对续传很重要.
	memset(tempStr, 0, 1024);
	sprintf_s(tempStr, "Range: bytes=%d-\r\n", nPos);
	sTemp.append(tempStr);

	// LastLine: 空行.
	memset(tempStr, 0, 1024);
	sprintf_s(tempStr, "\r\n");
	sTemp.append(tempStr);
	if (!SocketSend(hSocket, sTemp)) return FALSE;

	// 取得http头.
	int i = GetHttpHeader(hSocket, cTmpBuffer);
	if (!i)
	{
		//BSWriteLog("获取HTTP头出错!");
		return 0;
	}

	// 如果取得的http头含有404等字样，则表示连接出问题.
	sTemp = cTmpBuffer;
	size_t sPos = sTemp.find_first_of("\r\n");
	if (sPos == string::npos)
		return FALSE;
	sTemp = sTemp.substr(0, sPos);
	if (sTemp.find("404") != string::npos) 
		return FALSE;

	// 得到待下载文件的大小.
	m_nFileLength = GetFileLength(cTmpBuffer);

	//打印reponse header
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
		//BSWriteLog("发送请求失败!");
		return FALSE;
	}
	return TRUE;
}


CDealSocket dealsocket;

//---------------------------------------------------------------------------
CDealSocket::CDealSocket()
{
	// 套接字初始化.
	WORD wVersionRequested = MAKEWORD(1, 1);
	WSADATA wsaData;

	// 初始化WinSock.
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		//BSWriteLog("WSAStartup");
		return;
	}

	// 检查 WinSock 版本.
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
	// 释放WinSock.
	WSACleanup();
}

//---------------------------------------------------------------------------
string CDealSocket::GetResponse(SOCKET hSock)
{
	char szBufferA[MAX_RECV_LEN];  	// ASCII字符串. 
	int	iReturn;					// recv函数返回的值.

	char szError[1024] = {0};
	string strPlus = "";

	while (1)
	{
		// 从套接字接收资料.
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
	SOCKADDR_IN saServer;          // 服务器套接字地址.
	PHOSTENT phostent = NULL;	   // 指向HOSTENT结构指针.

								   // 创建一个绑定到服务器的TCP/IP套接字.
	if ((hSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		//BSWriteLog("Allocating socket failed. Error: %d", WSAGetLastError());
		return INVALID_SOCKET;
	}

	// 使用TCP/IP协议.
	saServer.sin_family = AF_INET;

	// 获取与主机相关的信息.
	if ((phostent = gethostbyname(host.c_str())) == NULL)
	{
		//BSWriteLog("Unable to get the host name. Error: %d", WSAGetLastError());
		closesocket(hSocket);
		return INVALID_SOCKET;
	}

	// 给套接字IP地址赋值.
	memcpy((char *)&(saServer.sin_addr),
		phostent->h_addr,
		phostent->h_length);

	// 设定套接字端口号.
	saServer.sin_port = htons(port);

	// 建立到服务器的套接字连接.
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
	SOCKET ListenSocket = INVALID_SOCKET;	// 监听套接字.
	SOCKADDR_IN local_sin;				    // 本地套接字地址.

											// 创建TCP/IP套接字.
	if ((ListenSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		//BSWriteLog("Allocating socket failed. Error: %d", WSAGetLastError());
		return INVALID_SOCKET;
	}

	// 给套接字信息结构赋值.
	local_sin.sin_family = AF_INET;
	local_sin.sin_port = htons(port);
	local_sin.sin_addr.s_addr = htonl(INADDR_ANY);

	// 进行本机地址与监听套接字绑定.
	//bind(ListenSocket, (sockaddr *)&local_sin, sizeof(local_sin))
	if (SOCKET_ERROR == ::bind(ListenSocket,(sockaddr*)&local_sin,sizeof(local_sin)))
	{
		//BSWriteLog("Binding socket failed. Error: %d", WSAGetLastError());
		closesocket(ListenSocket);
		return INVALID_SOCKET;
	}

	// 建立套接字对外部连接的监听.
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
		// 打开已有文件进行写数据.
		fp = fopen(lpszFileName, "r+b");
	}
	else {
		// 创建新文件进行写数据.
		fp = fopen(lpszFileName, "w+b");
	}

	return fp;
}

//---------------------------------------------------------------------------
DWORD CMyFile::GetFileSizeByName(LPCTSTR lpszFileName)
{
	if (!FileExists(lpszFileName)) return 0;
	struct _stat ST;
	// 获取文件长度.
	_stat(lpszFileName, &ST);
	UINT nFilesize = ST.st_size;
	return nFilesize;
}

//---------------------------------------------------------------------------
// 从全程文件名中提取短文件名.
string CMyFile::GetShortFileName(LPCSTR lpszFullPathName)
{
	string strFileName = lpszFullPathName;
	string strShortName;
	int n = strFileName.rfind('/');
	strShortName = strFileName.substr(n + 1, strFileName.size() - 1);
	return strShortName;
}
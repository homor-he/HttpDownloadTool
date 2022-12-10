//#include "stdafx.h"
#include "DownloadHelper.h"
#include "Mydownload.h"
#include "ChineseCode.h"
#include <io.h>
#include "DownloadHelpMgr.h"

bool ParseURL(string & URL, string &host, string &path, string &filename)
{
	string str = URL;
	int n = 0;
	n = str.find(':');
	string httpType = str.substr(0, n);
	string strFind;
	if (httpType == "http")
	{
		strFind = "http://";
	}
	else if (httpType == "https")
	{
		strFind = "https://";
	}
	n = str.find(strFind);
	if (n != -1) 
	{
		str = str.substr(n + strFind.size(), URL.size() - 1);
	}
	else
	{
		//BSWriteLog("URL:%s error, stop download!", URL.c_str());
		return false;
	}

	n = str.find('/');
	host = str.substr(0, n);
	n = str.rfind('/', URL.size() - 1);
	path = URL.substr(0, n + strFind.size() + 1);
	filename = URL.substr(n + strFind.size()+ 1, str.size() - 1);
	return true;
}

// 下载文件入口函数
int fnMyDownload(string strUrl,
	string strWriteFileName,
	unsigned long *& downloaded,
	unsigned long & totalSize,
	string strProxy,
	int nProxyPort,
	int nThread,
	DWORD dwCallerID
)
{
	CHttpGet cHttpGet;
	//请求url地址,域名
	string strHostAddr;
	//http资源路径
	string strHttpAddr;
	//资源文件名
	string strHttpFilename;

	//用于跟踪实时下载数据
	//dwCallerID == 0,使用rdownloaded全局变量
	//dwCallerID != 0,使用gbs_lsDownloded全局变量
	if (dwCallerID == 0)
	{
		downloaded = &rdownloaded;
	}
	else
	{
		downloaded = &gbs_lsDownloded[dwCallerID - 1];
	}

	if (!ParseURL(strUrl, strHostAddr, strHttpAddr, strHttpFilename))
		return -1;
	//本地文件名还是保留GB2312
	strWriteFileName += strHttpFilename;	

	//http默认是UTF8编码，所以把中文编码为UTF8
	string remoteFileName;		
	ChineseCode::GB2312ToUTF_8(remoteFileName, (char*)strHttpFilename.c_str(), strHttpFilename.size());
	strHttpFilename = remoteFileName.c_str();
	//服务器的文件路径  "http://www.123.com/123/"
	string remoteFilePath;		
	ChineseCode::GB2312ToUTF_8(remoteFilePath, (char*)strHttpAddr.c_str(), strHttpAddr.size());
	strHttpAddr = remoteFilePath.c_str();

	if (strProxy != "") 
	{
		if (!cHttpGet.HttpDownLoadProxy(strProxy, nProxyPort, strHostAddr, strHttpAddr, strHttpFilename, strWriteFileName, nThread, totalSize,dwCallerID))
			return 0;
	}
	else 
	{
		if (!cHttpGet.HttpDownLoadNonProxy(strUrl, strHostAddr, strHttpAddr, strHttpFilename, strWriteFileName, nThread, totalSize,dwCallerID))
			return 0;
	}
	return 1;
}


//---------------------------------------------------------------------------------
DownloadProgress::DownloadProgress()
{
	m_downloadPara = { 0 };
	onProgress = NULL;
	onProgressPara = NULL;
	m_phEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

void * DownloadProgress::Run(void * para)
{

	unsigned long temp = 0;
	while (!m_bIsExit)
	{
		WaitForSingleObject(m_phEvent, INFINITE);
		PDOWNLOADPARA downloadPara = (PDOWNLOADPARA)para;
		if (m_bIsExit |(NULL == downloadPara)) break;
		unsigned long downloaded = *(*downloadPara->pDownloaded);
		downloadPara->progress = (downloaded * 100) / (*downloadPara->pTotalSize + 1);
		downloadPara->downloadSpeed = (downloaded - temp)*1.0f / 1024;
		temp = downloaded;
		//BSWriteLog("资源：%s,当前已完成:%d%%,下载速度 %.2lf KB/S",downloadPara->downloadingUrl, downloadPara->progress, downloadPara->downloadSpeed);
		if (downloaded >= (*downloadPara->pTotalSize - 1))
		{
			//BSWriteLog("下载结束！");
			return 0;
		}
		if ((NULL != onProgress) && (NULL != onProgressPara))
			onProgress(onProgressPara);
		Sleep(1000);
	}
	return 0;
}

bool DownloadProgress::startTraceProgress()
{
	m_bIsExit = false;
	this->Start((void*)&m_downloadPara);
	return true;
}

void DownloadProgress::Pause()
{
	ResetEvent(m_phEvent);
}

void DownloadProgress::GoOn()
{
	SetEvent(m_phEvent);
}

//---------------------------------------------------------------------------------
DownloadHelper::DownloadHelper()
{
	m_index = 0;
	onFinish = NULL;
	onFinishPara = NULL;
	onFinishEachFile = NULL;
	onFinishEachFilePara = NULL;
	m_isDownloading = false;
	m_phEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	::InitializeCriticalSection(&m_vecLock);
}

DownloadHelper::~DownloadHelper()
{
	ThreadExit();
	GoOn();
	Join();
	CloseHandle(m_phEvent);
	::DeleteCriticalSection(&m_vecLock);
}

void DownloadHelper::GoOn()
{
	SetEvent(m_phEvent);
}


void DownloadHelper::SetIndex(DWORD index)
{
	m_index = index;
}

//添加下载任务，以传入的url作为唯一标识符
bool DownloadHelper::AddDownloadTask(const char* remoteUrl, const char* localFolder)
{
	string remoteUrlString(remoteUrl);
	string localFolderString(localFolder);

	
	if (!ExistInVector(m_downloadListRemoteURLs, remoteUrlString)) 
	{
		::EnterCriticalSection(&m_vecLock);
		m_downloadListRemoteURLs.emplace_back(remoteUrlString);
		m_downloadListLocalFolders.emplace_back(localFolderString);
		::LeaveCriticalSection(&m_vecLock);
		SetEvent(m_phEvent);
		return true;
	}
	else 
	{
		//::LeaveCriticalSection(&m_vecLock);
		return false;
	}
}

//判断字符串是否已经在vector<string>中出现
bool DownloadHelper::ExistInVector(vector<string>& array, string& str)
{
	::EnterCriticalSection(&m_vecLock);
	for (DWORD k = 0; k<array.size(); k++)
	{
		if (array[k].compare(str) == 0)
		{
			::LeaveCriticalSection(&m_vecLock);
			return true;
		}
			
	}
	::LeaveCriticalSection(&m_vecLock);
	return false;
}

//开始下载
bool DownloadHelper::StartDownload()
{
	this->Start();
	return true;
}

//多线程重构函数
void * DownloadHelper::Run(void *)
{
	//分配空间，用于跟踪。
	unsigned long temp = 0;
	unsigned long *downloaded = &temp;
	unsigned long totalSize = 1024;
	//DownloadProgress downloadProgress;
	//m_downloadProgress.m_downloadPara = {0};
	m_downloadProgress.m_downloadPara.pDownloaded = &downloaded;
	m_downloadProgress.m_downloadPara.pTotalSize = &totalSize;
	m_downloadProgress.startTraceProgress();
	while (!m_bIsExit && (NULL != m_phEvent))
	{
		WaitForSingleObject(m_phEvent, INFINITE);
		if (m_bIsExit) 
			break;
		while (GetDownloadListRemoteURLSize()>0)
		{
			string remoteUrl = GetDownloadRemoteURLIndexStr(0);
			string localFolder = GetDownloadLocalFolderIndexStr(0);
			m_isDownloading = true;
			*downloaded = 0;
			//BSWriteLog("url下载路径：%s，开始下载", remoteUrl.c_str());
			memset(m_downloadProgress.m_downloadPara.downloadingUrl, 0, 1024);
			memcpy(m_downloadProgress.m_downloadPara.downloadingUrl, remoteUrl.c_str(), 1024);
			//开启另一个线程跟踪下载进度
			m_downloadProgress.GoOn();
			//默认3线程下载，可以修改，但必须保持不变，因为断点续传需要前后两次线程数一致
			while (fnMyDownload(remoteUrl, localFolder, downloaded, totalSize, "", 0, THREAD_COUNT, m_index) != -1)
			{
				//阻塞式，直到下载成功或者网络出错才跳出
				if (!Exist(0))
				{
					//文件不存在，表示下载中断
					//BSWriteLog("网络中断，%d秒后重连...", RECONNECT_INTERVAL / 1000);
					//延时n秒后重连
					Sleep(RECONNECT_INTERVAL);
				}
				else
				{
					//下载成功，删除第一个任务
					//BSWriteLog("http请求：%s,资源已下载成功", remoteUrl.data());
					::EnterCriticalSection(&m_vecLock);
					vector<string>::iterator startIterator = m_downloadListRemoteURLs.begin();
					m_downloadListRemoteURLs.erase(startIterator);
					if ((NULL != onFinishEachFile) && (NULL != onFinishEachFilePara))
						onFinishEachFile(onFinishEachFilePara);
					startIterator = m_downloadListLocalFolders.begin();
					m_downloadListLocalFolders.erase(startIterator);
					::LeaveCriticalSection(&m_vecLock);
					break;
				}
				if ((GetDownloadListRemoteURLSize() < 1) | (GetDownloadListLocalFolderSize() < 1))
					break;
			}
			m_downloadProgress.Pause();
			m_isDownloading = false;
		}
		if ((onFinish != NULL) && (NULL!= onFinishPara))
			onFinish(onFinishPara);
		ResetEvent(m_phEvent);
	}
	m_downloadProgress.ThreadExit();
	m_downloadProgress.GoOn();
	m_downloadProgress.CloseThreadHandle();
	return NULL;
}

//判断下载列表的文件是否已经存在
//传入index是downloadListRemoteURLs的下标
bool DownloadHelper::Exist(int index)
{
	string fileName = m_downloadListRemoteURLs[index].substr(m_downloadListRemoteURLs[index].find_last_of("/") + 1);
	string file(m_downloadListLocalFolders[index].data());	
	file.append(fileName);
	return (_access(file.data(), 0) == 0);;
}

int DownloadHelper::GetDownloadListRemoteURLSize()
{
	int size = 0;
	::EnterCriticalSection(&m_vecLock);
	size = m_downloadListRemoteURLs.size();
	::LeaveCriticalSection(&m_vecLock);
	return size;
}

string DownloadHelper::GetDownloadRemoteURLIndexStr(int index)
{
	string str = "";
	::EnterCriticalSection(&m_vecLock);
	str = m_downloadListRemoteURLs[index];
	::LeaveCriticalSection(&m_vecLock);
	return str;
}

int DownloadHelper::GetDownloadListLocalFolderSize()
{
	int size = 0;
	::EnterCriticalSection(&m_vecLock);
	size = m_downloadListLocalFolders.size();
	::LeaveCriticalSection(&m_vecLock);
	return size;
}

string DownloadHelper::GetDownloadLocalFolderIndexStr(int index)
{
	string str = "";
	::EnterCriticalSection(&m_vecLock);
	str = m_downloadListLocalFolders[index];
	::LeaveCriticalSection(&m_vecLock);
	return str;
}

void DownloadHelper::SetOnFinishPara(void * para)
{
	onFinishPara = para;
}

//传入函数指针，下载完成后调用
void DownloadHelper::SetOnFinish(void(*func)(void *)) {
	onFinish = func;
}

void DownloadHelper::SetOnFinishEachFilePara(void * para)
{
	onFinishEachFilePara = para;
}

void DownloadHelper::SetOnFinishEachFile(void(*func)(void*))
{
	onFinishEachFile = func;
}

void DownloadHelper::SetOnProgress(void(*func)(void *))
{
	m_downloadProgress.onProgress = func;
}

void DownloadHelper::SetOnProgressPara(void * para)
{
	m_downloadProgress.onProgressPara = para;
}

bool DownloadHelper::IsDownloading()
{
	return m_isDownloading;
}

char * DownloadHelper::GetDownloadingUrl()
{
	if (m_isDownloading)
	{
		if (NULL != m_downloadProgress.m_downloadPara.downloadingUrl)
			return m_downloadProgress.m_downloadPara.downloadingUrl;
	}
	else
	{
		//BSWriteLog("下载已结束，获取正在下载的url失败！");
	}
	return NULL;
}

DWORD DownloadHelper::GetDownloadProgress()
{
	if (m_isDownloading)
		return m_downloadProgress.m_downloadPara.progress;
	else
	{
		//BSWriteLog("下载已结束，完成进度：100%%！");
		return 100;
	}
}

float DownloadHelper::GetDownloadSpeed()
{
	if(m_isDownloading)
		return m_downloadProgress.m_downloadPara.downloadSpeed;
	else
	{
		//BSWriteLog("下载已结束，下载速度：0KB/S！");
	}
	return 0;
}




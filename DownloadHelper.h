#ifndef __DOWNLOADHELPER_H__
#define __DOWNLOADHELPER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>
#include <string>
#include <iostream>
#include "Thread.h"
using namespace std;

//每个任务线程数
#define THREAD_COUNT 3
//重连时间
#define RECONNECT_INTERVAL 1000	

//bool existInVector(vector<string>& array, string& str);

//http请求,有断点续传功能,大小型文件都可用该方法下载,直接调用fnMyDownload函数即可
int fnMyDownload(
	string strUrl,
	string strWriteFileName,
	unsigned long *& downloaded,
	unsigned long & totalSize,
	string strProxy,
	int nProxyPort,
	int nThread,
	DWORD dwCallerID
);

typedef struct DownloadPara
{
	char	         downloadingUrl[1024];
	unsigned long ** pDownloaded;
	unsigned long *  pTotalSize;
	DWORD			 progress;
	float			 downloadSpeed;
}DOWNLOADPARA, *PDOWNLOADPARA;

class DownloadProgress : public Thread
{
public:
	DownloadProgress();
	void * Run(void * para);
	//跟踪下载进度
	bool startTraceProgress();
	void Pause();
	void GoOn();
public:
	void(*onProgress)(void*);
	void* onProgressPara;	
public:
	DOWNLOADPARA m_downloadPara;
private:
	HANDLE	m_phEvent;
};

class DownloadHelper : public Thread
{
public:
	DownloadHelper();
	virtual ~DownloadHelper();

	void * Run(void *);
public:
	bool StartDownload();
	//例：remoteUrl："http://www.abc.com/123.jpg"
	//	  localFolder: "f:\\download\\123.jpg"
	bool AddDownloadTask(const char* remoteUrl, const char* localFolder);
	//DownloadHelper对象完成下载任务的回调
	void SetOnFinishPara(void * para);
	void SetOnFinish(void(*func)(void*));
	//每完成下载一个文件的回调
	void SetOnFinishEachFilePara(void * para);
	void SetOnFinishEachFile(void(*func)(void*));
	//正在下载文件的进度回调
	void SetOnProgress(void(*func)(void*));
	void SetOnProgressPara(void* para);
	//获得DownloadHelper是否处于下载状态
	bool  IsDownloading();
	//获取当前正在下载的url地址
	char* GetDownloadingUrl(); 
	//获取当前正在下载的url资源的进度
	DWORD GetDownloadProgress();
	//获取当前正在下载的url资源的下载速度
	float GetDownloadSpeed();

	void GoOn();
	bool ExistInVector(vector<string>& array, string& str);
	void SetIndex(DWORD index);
private:
	//判断下载列表的文件是否已经存在，传入index是downloadListRemoteURLs的下标
	bool Exist(int index);
	int GetDownloadListRemoteURLSize();
	string GetDownloadRemoteURLIndexStr(int index);
	int GetDownloadListLocalFolderSize();
	string GetDownloadLocalFolderIndexStr(int index);
private:
	CRITICAL_SECTION m_vecLock;
	//文件网络url路径
	vector<string> m_downloadListRemoteURLs;
	//文件在本地保存的目录
	vector<string> m_downloadListLocalFolders;
	//所有队列里的下载任务都完成后回调
	void(*onFinish)(void*);
	void* onFinishPara;
	//每一个任务下载后完成的回调
	void(*onFinishEachFile)(void*);
	void* onFinishEachFilePara;
	//ChineseCode chineseCode;
	DownloadProgress m_downloadProgress;
	//是否下载的状态
	bool m_isDownloading;
	//用于区分线程对象
	DWORD	m_index;
	HANDLE	m_phEvent;
};

#endif // !__DOWNLOADHELPER_H__
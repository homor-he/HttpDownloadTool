#ifndef __DOWNLOADHELPMGR_H__
#define __DOWNLOADHELPMGR_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _DOWNLOADDLL
#define REGDLL_API __declspec(dllexport)
#else
#define REGDLL_API __declspec(dllimport)
#endif

#include "DownloadHelper.h"
#include "BSPtrList.h"

#define MAX_DOWNLOADHELPER_COUNT			5

typedef struct TaskStr
{
	char remoteUrl[256];
	char localFolder[256];

	TaskStr()
	{
		memset(localFolder, 0, 256);
		memset(remoteUrl, 0, 256);
	}
}TASKSTR, *PTASKSTR;

//只需要下载任务逐个进行时，可单独创建一个DownloadHelper，并把下载任务使用addDownloadTask放进队列中
//想要多个下载任务同时进行时，必须使用DownloadHelpMgr，且最多有5个DownloadHelper同时进行
 class REGDLL_API DownloadHelpMgr
{
public:
	DownloadHelpMgr();
	virtual ~DownloadHelpMgr();

	//创建count数量的下载器,一个下载器对应一个线程
	BOOL CreateDownloadHelper(DWORD count);
	//不管管理器StartDownload()是否调用，都可调用AddTask。
	//一：已调用StartDownload()，再调用AddTask将直接下载;
	//二：未调用StartDownload()，调用AddTask只是添加额外的下载任务到队列中
	//例：remoteUrl："http://www.abc.com/123.jpg"
	//	  localFolder: "F:\\download\\"
	//本地文件路径 F:\\download\\123.jpg
	BOOL AddTask(const char* remoteUrl,const char* localFolder);
	DownloadHelper* GetNextDownloadHelper();
	//启动管理器开始下载
	BOOL StartDownload();
	BOOL GetAllDownloadHelper(CBSPtrList<DownloadHelper>& pList);
	BOOL GetBusyDownloadHelper(CBSPtrList<DownloadHelper>& pList);
	BOOL GetIdleDownloadHelper(CBSPtrList<DownloadHelper>& pList);
	//每一个DownloadHelper对象完成下载任务的回调
	void SetOnFinishPara(void * para);                //自定义参数
	void SetOnFinish(void(*func)(void*));			  //自定义回调函数
	//每完成下载一个文件的回调
	void SetOnFinishEachFilePara(void * para);		  //自定义参数
	void SetOnFinishEachFile(void(*func)(void*));	  //自定义回调函数
	//正在下载文件的进度回调
	void SetOnProgressPara(void* para);				  //自定义参数
	void SetOnProgress(void(*func)(void*));			  //自定义回调函数
private:
	void(*onFinish)(void*);
	void* onFinishPara;
	//每一个任务下载后完成的回调
	void(*onFinishEachFile)(const char* downloadFile);
	void* onFinishEachFilePara;
private:
	DWORD				m_dwCount;
	DWORD				m_dwNext;
	DownloadHelper*		m_lsDownloadHelper[MAX_DOWNLOADHELPER_COUNT];
	CRITICAL_SECTION	m_csTaskStr;
	CBSPtrList<TaskStr> m_lsTaskStr;
	//BOOL				m_start;
};

extern vector<DWORD> gbs_lsDownloded;

#undef REGDLL_API
#endif // !__DOWNLOADHELPMGR_H__

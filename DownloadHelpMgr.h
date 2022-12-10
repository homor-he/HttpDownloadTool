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

//ֻ��Ҫ���������������ʱ���ɵ�������һ��DownloadHelper��������������ʹ��addDownloadTask�Ž�������
//��Ҫ�����������ͬʱ����ʱ������ʹ��DownloadHelpMgr���������5��DownloadHelperͬʱ����
 class REGDLL_API DownloadHelpMgr
{
public:
	DownloadHelpMgr();
	virtual ~DownloadHelpMgr();

	//����count������������,һ����������Ӧһ���߳�
	BOOL CreateDownloadHelper(DWORD count);
	//���ܹ�����StartDownload()�Ƿ���ã����ɵ���AddTask��
	//һ���ѵ���StartDownload()���ٵ���AddTask��ֱ������;
	//����δ����StartDownload()������AddTaskֻ����Ӷ�����������񵽶�����
	//����remoteUrl��"http://www.abc.com/123.jpg"
	//	  localFolder: "F:\\download\\"
	//�����ļ�·�� F:\\download\\123.jpg
	BOOL AddTask(const char* remoteUrl,const char* localFolder);
	DownloadHelper* GetNextDownloadHelper();
	//������������ʼ����
	BOOL StartDownload();
	BOOL GetAllDownloadHelper(CBSPtrList<DownloadHelper>& pList);
	BOOL GetBusyDownloadHelper(CBSPtrList<DownloadHelper>& pList);
	BOOL GetIdleDownloadHelper(CBSPtrList<DownloadHelper>& pList);
	//ÿһ��DownloadHelper���������������Ļص�
	void SetOnFinishPara(void * para);                //�Զ������
	void SetOnFinish(void(*func)(void*));			  //�Զ���ص�����
	//ÿ�������һ���ļ��Ļص�
	void SetOnFinishEachFilePara(void * para);		  //�Զ������
	void SetOnFinishEachFile(void(*func)(void*));	  //�Զ���ص�����
	//���������ļ��Ľ��Ȼص�
	void SetOnProgressPara(void* para);				  //�Զ������
	void SetOnProgress(void(*func)(void*));			  //�Զ���ص�����
private:
	void(*onFinish)(void*);
	void* onFinishPara;
	//ÿһ���������غ���ɵĻص�
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

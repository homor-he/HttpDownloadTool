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

//ÿ�������߳���
#define THREAD_COUNT 3
//����ʱ��
#define RECONNECT_INTERVAL 1000	

//bool existInVector(vector<string>& array, string& str);

//http����,�жϵ���������,��С���ļ������ø÷�������,ֱ�ӵ���fnMyDownload��������
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
	//�������ؽ���
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
	//����remoteUrl��"http://www.abc.com/123.jpg"
	//	  localFolder: "f:\\download\\123.jpg"
	bool AddDownloadTask(const char* remoteUrl, const char* localFolder);
	//DownloadHelper���������������Ļص�
	void SetOnFinishPara(void * para);
	void SetOnFinish(void(*func)(void*));
	//ÿ�������һ���ļ��Ļص�
	void SetOnFinishEachFilePara(void * para);
	void SetOnFinishEachFile(void(*func)(void*));
	//���������ļ��Ľ��Ȼص�
	void SetOnProgress(void(*func)(void*));
	void SetOnProgressPara(void* para);
	//���DownloadHelper�Ƿ�������״̬
	bool  IsDownloading();
	//��ȡ��ǰ�������ص�url��ַ
	char* GetDownloadingUrl(); 
	//��ȡ��ǰ�������ص�url��Դ�Ľ���
	DWORD GetDownloadProgress();
	//��ȡ��ǰ�������ص�url��Դ�������ٶ�
	float GetDownloadSpeed();

	void GoOn();
	bool ExistInVector(vector<string>& array, string& str);
	void SetIndex(DWORD index);
private:
	//�ж������б���ļ��Ƿ��Ѿ����ڣ�����index��downloadListRemoteURLs���±�
	bool Exist(int index);
	int GetDownloadListRemoteURLSize();
	string GetDownloadRemoteURLIndexStr(int index);
	int GetDownloadListLocalFolderSize();
	string GetDownloadLocalFolderIndexStr(int index);
private:
	CRITICAL_SECTION m_vecLock;
	//�ļ�����url·��
	vector<string> m_downloadListRemoteURLs;
	//�ļ��ڱ��ر����Ŀ¼
	vector<string> m_downloadListLocalFolders;
	//���ж����������������ɺ�ص�
	void(*onFinish)(void*);
	void* onFinishPara;
	//ÿһ���������غ���ɵĻص�
	void(*onFinishEachFile)(void*);
	void* onFinishEachFilePara;
	//ChineseCode chineseCode;
	DownloadProgress m_downloadProgress;
	//�Ƿ����ص�״̬
	bool m_isDownloading;
	//���������̶߳���
	DWORD	m_index;
	HANDLE	m_phEvent;
};

#endif // !__DOWNLOADHELPER_H__
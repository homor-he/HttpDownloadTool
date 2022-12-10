#ifndef _THREAD_SPECIFICAL_H__
#define _THREAD_SPECIFICAL_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include   <windows.h>   

//static unsigned int __stdcall threadFunction(void *);

class Thread 
{
public:
	Thread();
	virtual ~Thread();
	int Start(void * = NULL);			//�߳����������������������������ָ�롣
	//virtual void stop();
	void* Join();						//�ȴ���ǰ�߳̽���
	void Detach();						//���ȴ���ǰ�߳�
	void ThreadExit();
	void CloseThreadHandle();
	static void Sleep(unsigned int);	//�õ�ǰ�߳����߸���ʱ�䣬��λΪ����
protected:
	static unsigned int __stdcall threadFunction(void *);
	virtual void * Run(void *) = 0;		//����ʵ���߳�����̺߳�������
protected:
	bool		 m_bIsExit;				//�Ƿ��˳�
private:
	HANDLE		 m_threadHandle;
	bool		 m_started;
	bool		 m_detached;
	void*		 m_param;
	unsigned int m_threadID;
};

#endif

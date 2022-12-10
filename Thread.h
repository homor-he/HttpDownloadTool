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
	int Start(void * = NULL);			//线程启动函数，其输入参数是无类型指针。
	//virtual void stop();
	void* Join();						//等待当前线程结束
	void Detach();						//不等待当前线程
	void ThreadExit();
	void CloseThreadHandle();
	static void Sleep(unsigned int);	//让当前线程休眠给定时间，单位为毫秒
protected:
	static unsigned int __stdcall threadFunction(void *);
	virtual void * Run(void *) = 0;		//用于实现线程类的线程函数调用
protected:
	bool		 m_bIsExit;				//是否退出
private:
	HANDLE		 m_threadHandle;
	bool		 m_started;
	bool		 m_detached;
	void*		 m_param;
	unsigned int m_threadID;
};

#endif

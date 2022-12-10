#include <process.h>
#include "Thread.h"
//#include "stdafx.h"  

unsigned int __stdcall Thread::threadFunction(void * object)
{
	Thread * thread = (Thread *)object;
	return (unsigned int)thread->Run(thread->m_param);
}

Thread::Thread()
{
	m_started = false;
	m_detached = false;
	m_bIsExit = false;
}

Thread::~Thread()
{
	//stop();
}

int Thread::Start(void* pra)
{
	if (!m_started)
	{
		m_param = pra;
		if (m_threadHandle = (HANDLE)_beginthreadex(NULL, 0, threadFunction, (void*)this, 0, &m_threadID))
		{
			if (m_detached)
			{
				CloseHandle(m_threadHandle);
			}
			m_started = true;
		}
	}
	return m_started;
}

//wait for current thread to end.
void * Thread::Join()
{
	DWORD status = (DWORD)NULL;
	if (m_started && !m_detached)
	{
		WaitForSingleObject(m_threadHandle, INFINITE);
		GetExitCodeThread(m_threadHandle, &status);
		CloseHandle(m_threadHandle);
		m_detached = true;
	}

	return (void *)status;
}

void Thread::Detach()
{
	if (m_started && !m_detached)
	{
		CloseHandle(m_threadHandle);
	}
	m_detached = true;
}

void Thread::ThreadExit()
{
	m_bIsExit = true;
}

void Thread::CloseThreadHandle()
{
	CloseHandle(m_threadHandle);
}

//void Thread::stop()
//{
//	if (started && !detached)
//	{
//		TerminateThread(threadHandle, 0);
//
//		//Closing a thread handle does not terminate 
//		//the associated thread. 
//		//To remove a thread object, you must terminate the thread, 
//		//then close all handles to the thread.
//		//The thread object remains in the system until 
//		//the thread has terminated and all handles to it have been 
//		//closed through a call to CloseHandle
//		CloseHandle(threadHandle);
//		detached = true;
//	}
//}

void Thread::Sleep(unsigned int delay)
{
	::Sleep(delay);
}

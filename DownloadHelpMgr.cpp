#include "DownloadHelpMgr.h"

vector<DWORD> gbs_lsDownloded;

DownloadHelpMgr::DownloadHelpMgr()
{
	m_dwCount = 0;
	m_dwNext = 0;
	memset(&m_lsDownloadHelper, 0, sizeof(m_lsDownloadHelper));
	::InitializeCriticalSection(&m_csTaskStr);
	//m_start = false;
}

DownloadHelpMgr::~DownloadHelpMgr()
{
	//StopAllDownloadHelper();
	for (DWORD i = 0; i < m_dwCount; ++i)
	{
		delete m_lsDownloadHelper[i];
		m_lsDownloadHelper[i] = NULL;
	}

	::EnterCriticalSection(&m_csTaskStr);
	m_lsTaskStr.DeleteAll(TRUE);
	::LeaveCriticalSection(&m_csTaskStr);
	::DeleteCriticalSection(&m_csTaskStr);
}

BOOL DownloadHelpMgr::CreateDownloadHelper(DWORD count)
{
	if (count >= MAX_DOWNLOADHELPER_COUNT)
	{
		count = MAX_DOWNLOADHELPER_COUNT;
	}
	for (DWORD i = 0; i < count; ++i)
	{
		m_lsDownloadHelper[i] = new DownloadHelper;
		//Mgr创建DownloadHelper时，不存在index=0
		//index=0用于表示非Mgr创建的DownloadHelper唯一对象
		m_lsDownloadHelper[i]->SetIndex(i + 1);
		unsigned long downloaded = 0;
		gbs_lsDownloded.emplace_back(downloaded);
		m_dwCount++;
	}
	return TRUE;
}

BOOL DownloadHelpMgr::AddTask(const char * remoteUrl, const char * localFolder)
{
	TaskStr* pNewTaskStr = new TaskStr;
	memcpy_s(pNewTaskStr->remoteUrl, 256, remoteUrl, 256);
	memcpy_s(pNewTaskStr->localFolder, 256, localFolder, 256);
	::EnterCriticalSection(&m_csTaskStr);
	m_lsTaskStr.AddTail(pNewTaskStr);
	
	DownloadHelper * pDownloadHelp = NULL;
	int count = 0;
	while (NULL != (pDownloadHelp = GetNextDownloadHelper()))
	{
		if (!pDownloadHelp->IsDownloading())
		{
			PTASKSTR pTaskStr = m_lsTaskStr.GetHead();
			if (m_dwNext > 0)
			{
				gbs_lsDownloded[m_dwNext - 1] = 0;
			}
			else
			{
				gbs_lsDownloded[m_dwCount - 1] = 0;
			}
			
			pDownloadHelp->AddDownloadTask(pTaskStr->remoteUrl, pTaskStr->localFolder);
			if (m_lsTaskStr.RemoveHead())
			{
				delete pTaskStr;
				pTaskStr = NULL;
			}
			break;
		}

		if (count == m_dwCount)
		{
			while (m_lsTaskStr.GetSize() > 0)
			{
				PTASKSTR pTaskStr = m_lsTaskStr.GetHead();
				int random = rand() % m_dwCount;
				m_lsDownloadHelper[random]->AddDownloadTask(pTaskStr->remoteUrl, pTaskStr->localFolder);
				if (m_lsTaskStr.RemoveHead())
				{
					delete pTaskStr;
					pTaskStr = NULL;
				}
			}
			break;
		}
		count++;
	}
	::LeaveCriticalSection(&m_csTaskStr);
	return TRUE;
}

DownloadHelper * DownloadHelpMgr::GetNextDownloadHelper()
{
	DownloadHelper* pDownloadHelper = NULL;
	if (m_dwCount > 0)
	{
		pDownloadHelper = m_lsDownloadHelper[m_dwNext];
		m_dwNext = (m_dwNext + 1) % m_dwCount;
	}
	return pDownloadHelper;
}

BOOL DownloadHelpMgr::StartDownload()
{
	::EnterCriticalSection(&m_csTaskStr);
	for (int i = 0; i < m_lsTaskStr.GetSize(); ++i)
	{
		PTASKSTR pTaskStr = m_lsTaskStr.GetHead();
		DownloadHelper * pDownloadHelper = GetNextDownloadHelper();
		pDownloadHelper->AddDownloadTask(pTaskStr->remoteUrl, pTaskStr->localFolder);
		if (m_lsTaskStr.RemoveHead())
		{
			delete pTaskStr;
			pTaskStr = NULL;
		}
	}
	::LeaveCriticalSection(&m_csTaskStr);
	for (DWORD i =0;i<m_dwCount;++i)
	{
		m_lsDownloadHelper[i]->StartDownload();
	}
	//m_start = true;
	return TRUE;
}

BOOL DownloadHelpMgr::GetAllDownloadHelper(CBSPtrList<DownloadHelper>& pList)
{
	if (m_dwCount < 1) 
		return FALSE;

	for (DWORD i = 0; i < m_dwCount; ++i)
	{
		pList.AddTail(m_lsDownloadHelper[i]);
	}
	if (pList.GetSize() > 0)
		return TRUE;
	else
		return FALSE;
}

BOOL DownloadHelpMgr::GetBusyDownloadHelper(CBSPtrList<DownloadHelper>& pList)
{
	if (m_dwCount < 1)
		return FALSE;

	for (DWORD i = 0; i < m_dwCount; ++i)
	{
		if(m_lsDownloadHelper[i]->IsDownloading())
			pList.AddTail(m_lsDownloadHelper[i]);
	}
	if (pList.GetSize() > 0)
		return TRUE;
	else
		return FALSE;
}

BOOL DownloadHelpMgr::GetIdleDownloadHelper(CBSPtrList<DownloadHelper>& pList)
{
	if (m_dwCount < 1)
		return FALSE;

	for (DWORD i = 0; i < m_dwCount; ++i)
	{
		if (!m_lsDownloadHelper[i]->IsDownloading())
			pList.AddTail(m_lsDownloadHelper[i]);
	}
	if (pList.GetSize() > 0)
		return TRUE;
	else
		return FALSE;
}

void DownloadHelpMgr::SetOnFinishPara(void * para)
{
	for (DWORD i = 0; i < m_dwCount; ++i)
	{
		m_lsDownloadHelper[i]->SetOnFinishPara(para);
	}
}

void DownloadHelpMgr::SetOnFinish(void(*func)(void *))
{
	for (DWORD i = 0; i < m_dwCount; ++i)
	{
		m_lsDownloadHelper[i]->SetOnFinish(func);
	}
}

void DownloadHelpMgr::SetOnFinishEachFilePara(void * para)
{
	for (DWORD i = 0; i < m_dwCount; ++i)
	{
		m_lsDownloadHelper[i]->SetOnFinishEachFilePara(para);
	}
}

void DownloadHelpMgr::SetOnFinishEachFile(void(*func)(void*))
{
	for (DWORD i = 0; i < m_dwCount; ++i)
	{
		m_lsDownloadHelper[i]->SetOnFinishEachFile(func);
	}
}

void DownloadHelpMgr::SetOnProgressPara(void * para)
{
	for (DWORD i = 0; i < m_dwCount; ++i)
	{
		m_lsDownloadHelper[i]->SetOnProgressPara(para);
	}
}

void DownloadHelpMgr::SetOnProgress(void(*func)(void *))
{
	for (DWORD i = 0; i < m_dwCount; ++i)
	{
		m_lsDownloadHelper[i]->SetOnProgress(func);
	}
}

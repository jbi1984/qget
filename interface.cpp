#ifndef __TASK_INTERFACE_CPP
#define __TASK_INTERFACE_CPP

#include "interface.h"
ACE_Thread_Mutex INTERFACE_Mutex;
CTaskInterFace* CTaskInterFace::sington = NULL;

CTaskInterFace* CTaskInterFace::instance()
{
	if (NULL == sington)
	{
		sington = new CTaskInterFace;
		sington->init();
	}
	//std::cout<<"TaskInterFace instance created successfully"<<std::endl;
	return sington;
}

CTaskInterFace::CTaskInterFace()
{
	m_pFTPTaskManager = NULL;
	m_pHTTPTaskManager = NULL;
	m_TaskMap.clear();
}

bool CTaskInterFace::init()
{
	m_pFTPTaskManager = new CFTPDownloadTaskManager;
	m_pHTTPTaskManager = new CHttpDownloadTaskManager;
	return true;
}

/*bool CTaskInterFace::timer()
{
	return true;
}*/

long CTaskInterFace::AddTask(std::string strSourceURL, std::string strTargetURL, int iPiecesNum)//;//返回新任务的ID；
{
	std::cout<<"CTaskInterFace::AddTask"<<std::endl;
	long lTaskID = -1;
	if (std::string("") == strSourceURL || std::string("") == strTargetURL || 0 >= iPiecesNum)
	{
		std::cout<<"error parametor"<<std::endl;
		return lTaskID;
	}
	//get task type
	std::string strFTP = "FTP://";
	std::string strHTTP = "HTTP://";
	std::string strType = strSourceURL.substr(0,6);
		
	transform (strType.begin(), strType.end(), strType.begin(), toupper);
	if (strFTP == strType)
	{
		std::cout<<"CTaskInterFace::AddTask-> FTP: "<<std::endl;
		lTaskID = m_pFTPTaskManager->AddTask(strSourceURL/*, strTargetURL*/, iPiecesNum);
		std::cout<<"taskid: "<<lTaskID<<std::endl;
		m_TaskMap.insert(std::map<long, int>::value_type(lTaskID, 1));
		return lTaskID;
	}

	strType = strSourceURL.substr(0,7);
	transform (strType.begin(), strType.end(), strType.begin(), toupper);
	if (strHTTP == strType)
	{
		std::cout<<"CTaskInterFace::AddTask-> HTTP: "<<std::endl;
		lTaskID = m_pHTTPTaskManager->AddTask(strSourceURL, /*strTargetURL,*/ iPiecesNum);
		std::cout<<"taskid: "<<lTaskID<<std::endl;
		m_TaskMap.insert(std::map<long, int>::value_type(lTaskID, 2));
		
		return lTaskID;
	}
	else
	{
		std::cout<<"CTaskInterFace::AddTask-> NONE"<<std::endl;
		return -1;
	}
}

TASK_TYPE CTaskInterFace::GetTaskType(long lTaskID)
{
	//check the map and return the task type	
	if (lTaskID > 1024*4)
		return TASK_TYPE_FTP;
	return TASK_TYPE_HTTP;
}

//resume from memory
bool CTaskInterFace::StartTask (long lTaskID)
{
	if (lTaskID > 1024*4)
	{
		m_pFTPTaskManager->ResumeTask(lTaskID);
		return true;
	}
	m_pHTTPTaskManager->ResumeTask(lTaskID);
	return true;
}

bool CTaskInterFace::DeleteTask(long lTaskID)
{
	if (lTaskID > 1024*4)
	{
		m_pFTPTaskManager->DeleteTask(lTaskID);
		return true;
	}
	m_pHTTPTaskManager->DeleteTask(lTaskID);
	return true;
}
	
bool CTaskInterFace::PauseTask(long lTaskID)
{
	if (lTaskID > 1024*4)
	{
		m_pFTPTaskManager->SuspendTask(lTaskID);
		return true;
	}
	m_pHTTPTaskManager->SuspendTask(lTaskID);
	return true;
}

//stop task to disk
bool CTaskInterFace::StopTask(long lTaskID)
{
	if (lTaskID > 1024*4)
	{
		m_pFTPTaskManager->StopTask(lTaskID);
		return true;
	}
	m_pHTTPTaskManager->StopTask(lTaskID);
	return true;
}

//resume from disk
bool CTaskInterFace::ResumeTask(long lTaskID)
{
	true;
}

unsigned long CTaskInterFace::GetFileSize(long lTaskID)
{
	std::map<long, int>::iterator pos;
	pos = m_TaskMap.find(lTaskID);
	unsigned long uTmp = 0;
	if (pos->second == int(1))//ftp
	{
		std::cout<<"Find : FTPTASK_FILESIZE"<<std::endl;
		uTmp = m_pFTPTaskManager->GetFileSize(lTaskID);
		return uTmp;
	} 
	if (pos->second == int(2))//http
	{
		std::cout<<"Find : HTTPTASK_FILESIZE"<<std::endl;
		uTmp = m_pHTTPTaskManager->GetFileSize(lTaskID);
		return uTmp;
	}
	else
	{
		std::cout<<"wrong task id,please check!"<<std::endl;
		return 0;
	}
}

bool CTaskInterFace::GetSpeedAndRate(long lTaskID, double &speed, int &rate)
{
	std::cout<<"CTaskInterFace::GetSpeedAndRate"<<std::endl;
	ACE_Time_Value cur = ACE_OS::gettimeofday();
	std::map<long, int>::iterator pos;
	pos = m_TaskMap.find(lTaskID);
	if (pos->second == int(1))//ftp
	{
		std::cout<<"Find : FTPTASK"<<std::endl;
		m_pFTPTaskManager->GetSpeedAndRate(lTaskID, speed, rate);
	} 
	else
	if (pos->second == int(2))//http
	{
		std::cout<<"Find : HTTPTASK"<<std::endl;
		m_pHTTPTaskManager->GetSpeedAndRate(lTaskID, speed, rate);
	}
	else if (pos == m_TaskMap.end())
	{
		std::cout<<"wrong task id,please check!"<<std::endl;
	}
	return true;
}

CTaskInterFace::~CTaskInterFace()
{
	delete m_pFTPTaskManager;
	delete m_pHTTPTaskManager;
}

#endif

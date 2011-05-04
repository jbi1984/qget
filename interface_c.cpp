#ifndef __TASK_INTERFACE_H_C__
#define __TASK_INTERFACE_H_C__

#include "interface_c.h"
#include "interface.h"

static CTaskInterFace * pTaskInterFace = CTaskInterFace::instance();

long AddTask(char* strSourceURL, char* strTargetURL, int iPiecesNum)
{
	if (NULL == pTaskInterFace)
	{
		pTaskInterFace = CTaskInterFace::instance();
		pTaskInterFace->init();
	}
	return pTaskInterFace->AddTask(std::string(strSourceURL), std::string(strTargetURL), iPiecesNum);
}

bool StartTask (long lTaskID)
{
	pTaskInterFace->StartTask(lTaskID);
	return true;
}
bool DeleteTask(long lTaskID)
{
	pTaskInterFace->DeleteTask(lTaskID);
	return true;
}
bool PauseTask(long lTaskID)
{
	pTaskInterFace->PauseTask(lTaskID);
	return true;
}
bool StopTask(long lTaskID)
{
	pTaskInterFace->StopTask(lTaskID);
	true;
}
bool ResumeTask(long lTaskID)
{
	pTaskInterFace->ResumeTask(lTaskID);
	true;
}

bool GetSpeedAndRate(long lTaskID, double *speed, int *rate)
{
	pTaskInterFace->GetSpeedAndRate(lTaskID, *speed, *rate);
	return true;
}

unsigned long GetFileSize(long lTaskID)
{
	if (NULL == pTaskInterFace)
	{
		std::cout<<"Wrong status!Please check!"<<std::endl;
		return 0;
	}
	return pTaskInterFace->GetFileSize(lTaskID);
}

bool timer()
{
	return true;
	//return pTaskInterFace->timer();
}

#endif

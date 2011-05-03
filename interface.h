#ifndef __TASK_INTERFACE_H__
#define __TASK_INTERFACE_H__

#include "httpdecode.h"
#include "ftpdecode.h"

enum TASK_TYPE
{
	TASK_TYPE_FTP = 0,
	TASK_TYPE_HTTP = 1,
	TASK_TYPE_ERROR
};

//sington model
class CTaskInterFace
{
private:
	enum
	{
		TASK_NUM = 1024*4*4
	};
public:
	static CTaskInterFace* instance();
protected:
	CTaskInterFace();
public:
	bool init();
	//bool timer();
public:
	long AddTask(std::string strSourceURL, std::string strTargetURL, int iPiecesNum = 3);
	TASK_TYPE GetTaskType(long lTaskID);
	bool StartTask (long lTaskID);
	bool DeleteTask(long lTaskID);
	bool PauseTask(long lTaskID);
	bool StopTask(long lTaskID);
	bool ResumeTask(long lTaskID);
	bool GetSpeedAndRate(long lTaskID, double &speed, int &rate);
	unsigned long GetFileSize(long lTaskID);

	~CTaskInterFace();

public:
	static CTaskInterFace* sington;

private:
	CFTPDownloadTaskManager* m_pFTPTaskManager;
	CHttpDownloadTaskManager* m_pHTTPTaskManager;
	std::map<long, int> m_TaskMap;
};


/*class tasktimer: public ACE_Task<ACE_MT_SYNCH>
{
public:
	virtual int svc(void)
	{
		ACE_Reactor::instance()->run_reactor_event_loop(NULL);
	}
};*/

#endif //__TASK_INTERFACE_H__

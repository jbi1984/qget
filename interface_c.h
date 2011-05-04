#ifndef __TASK_INTERFACE_H_C_H__
#define __TASK_INTERFACE_H_C_H__

extern	long AddTask(char* strSourceURL, char* strTargetURL, int iPiecesNum = 6);
extern	bool StartTask (long lTaskID);
extern	bool DeleteTask(long lTaskID);
extern	bool PauseTask(long lTaskID);
extern	bool StopTask(long lTaskID);
extern	bool ResumeTask(long lTaskID);
extern	bool GetSpeedAndRate(long lTaskID, double *speed, int *rate);
extern	unsigned long GetFileSize(long lTaskID);
extern	bool timer();


#endif

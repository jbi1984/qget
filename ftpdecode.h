/*
 * 2011 -2021 ,All rights reserved.
 * Contact: heguanbo@gmail.com/gjhe@novell.com
 */
#ifndef _FTP_DECODE_HEADER
#define _FTP_DECODE_HEADER

//according to FRC0959
//Using ACE
//StartDate: 11/10,2007

#include "filestore.h"
#include <iostream>
#include <map>
#include <algorithm>
#include <iomanip>
#include "ace/SString.h"
#include "ace/FILE_IO.h"
#include "ace/Thread_Mutex.h"
#include "ace/Task_T.h"
#include "ace/SOCK_Connector.h"
#include "ace/Reactor.h"
#include "ace/FILE_Connector.h"
#include "ace/FILE_Addr.h"
#include "ace/SOCK_Acceptor.h"
#include "log.h"


class CFTPClient
{
public:
	//bool init();
	CFTPClient(int iPieceID);
	~CFTPClient();
	static ACE_INT64 GetFileSize(std::string strReqURL, std::string & strFileName, bool &bSupportREST);//for other func to call
	int SaveData(char *buf, ACE_INT64 uOffSet, int uDataLen);
	ACE_INT64 GetFTPFileSize();

	std::string m_strFileName;
	std::string m_strPieceFileName;
	std::string m_strDomain;
	CFileStore m_FileStore;
	bool m_IsThisPieceComplete;
	
	int m_pieces;
	int m_PieceID;
	ACE_INT64 m_RangeEnd;
	ACE_INT64 m_RangeStart;
	bool fflag;

	float m_fPercent;
	ACE_INT64 m_offset;
};



class CFTPConnector: public ACE_Task<ACE_MT_SYNCH>
{
public:
	~CFTPConnector();
	CFTPConnector(std::string strURL, ACE_Time_Value &timeout, ACE_INT64 uRangeStart, ACE_INT64 uPieceLen, int iPieceID, int iPieceNum);
	int connect ();
	virtual int svc(void);

public:
	CFTPClient *m_pFTPClient;

	ACE_INT64 m_uPieceLen;
	ACE_INT64 m_RangeStart;
private:
	
	ACE_SOCK_Connector m_Connector;
	ACE_SOCK_Connector m_ConnectorData;
	ACE_SOCK_Stream m_SockStream;
	ACE_SOCK_Acceptor m_Acceptor;
	ACE_SOCK_Stream m_SockStreamData;
	ACE_Time_Value m_TimeOut;
	std::string m_strURL;
	int m_PieceID;
	int m_PieceNum;
};


class CFTPDownloadTask: public ACE_Event_Handler
{
public:
	CFTPDownloadTask(std::string strURL, unsigned int iPieceNum = 5);
	unsigned int LaunchTask();//start threads
	virtual int handle_timeout(const ACE_Time_Value & current_time, const void * = 0);
	virtual int handle_signal(int signum, siginfo_t *t=0,ucontext_t *i =0);
	bool GetSpeedAndRate(double &speed, int &rate);
	ACE_INT64 GetFileSize();
        bool Start();
        bool Resume();
        bool Suspend();
        bool Delete();
        bool Stop();

	~CFTPDownloadTask();

public:
	bool m_bTaskComplete;
	int m_TimerID;
	CFileStore m_FileStore;
	static bool m_bFirstDisplay;
	double time;
	ACE_INT64 m_pre_offset;
	

//private:
	std::string m_strURL;
	std::string m_strFileName;
	unsigned int m_PieceNum;
	long m_TaskID;
	std::map<int, CFTPConnector*> m_ThreadMap;
	float m_fSpeed;
	int m_iPerc;
	ACE_INT64 m_FileSize;
	ACE_Time_Value m_StartTime;
};

class CFTPDownloadTaskManager : public ACE_Event_Handler
{
public:
	CFTPDownloadTaskManager();
	//int init ();
	long AddTask(std::string strURL, int iPiecesNum);
	bool StartTask(long lTaskID);
	bool DeleteTask(long lTaskID);
	bool SuspendTask(long lTaskID);
	bool ResumeTask(long lTaskID);
	bool StopTask(long lTaskID);
	bool GetSpeedAndRate(long lTaskID, double &speed, int &rate);
	ACE_INT64 GetFileSize(long lTaskID);
	CFTPDownloadTask* GetTask(long lTaskID);

	virtual int handle_timeout(const ACE_Time_Value & current_time, const void * = 0);
	~CFTPDownloadTaskManager();

private:
	long m_iTaskID;
	std::map<long, CFTPDownloadTask*> m_TaskMap;
};

#endif //_FTP_DECODE_HEADER

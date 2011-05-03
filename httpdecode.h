#ifndef _HTTP_DECODE_HEADER
#define _HTTP_DECODE_HEADER

//Using ACE
//StartDate: 10/10,2007
#include "filestore.h"
#include <iostream>
#include <map>
#include <algorithm>
#include <iomanip>
#include <stdio.h>
#include "ace/SString.h"
#include "ace/FILE_IO.h"
#include "ace/Thread_Mutex.h"
#include "ace/Task_T.h"
#include "ace/SOCK_Connector.h"
#include "ace/Reactor.h"
#include "ace/FILE_Connector.h"
#include "ace/FILE_Addr.h"

enum httpstat
{
	HTTP_TaskAdded,
	HTTP_Connecting,
	HTTP_Downloading,
	HTTP_ReConnecting,
	HTTP_Finish
};



class CHttpClient
{
public:
	//bool init();
	CHttpClient(int iPieceID);
	bool bEncodeRequest(std::string pstrRequestURL, std::string& strEncodedRequest, std::string strReqMethord, ACE_INT64 RangeStart =0, ACE_INT64 RangeEnd=0);
	//bool DecodeResponse(const std::string strTmpRes);//, std::string& strDecodedRes);
	unsigned int DecodeResponse(const char*resdata, int len);//, std::string& strDecodedRes);
	static ACE_INT64 HeadMethodForInfo(std::string &strURL, std::string & strFileName);
	static bool GetResCode(std::string strTmpRes, std::string &ResCode);
	static ACE_INT64 GetContentLen(std::string strTmpRes);
	static void Trim(std::string &strInput);
	static bool GetTaskStatus();
	bool UpdateRange(std::string &strEncodeReq, ACE_INT64 RangeStart, ACE_INT64 RangeEnd);
	bool bConnectClose(std::string res_header);
	bool bChunked(std::string res_header);

public:

//private:
	std::string m_strFileName;
	std::string m_strPieceFileName;
	std::string m_strDomain;
	CFileStore m_FileStore;
	ACE_INT64 m_offset;
	bool m_IsThisPieceComplete;
	int m_iPieceNum;
	int m_PieceID;
	ACE_INT64 m_RangeEnd;
	ACE_INT64 m_RangeStart;
	bool fflag;
	float m_fPrecent;
	ACE_INT64 m_iFileSize;
	bool m_bSupportHeadMethord;
	bool m_bChunk;
	ACE_INT64 m_ChunkLeftLen;
	bool m_bRevClose;
	bool m_jieli_updaterange;
};


class CHttpConnector: public ACE_Task<ACE_MT_SYNCH>
{
public:
	~CHttpConnector();
	CHttpConnector(std::string strURL, ACE_Time_Value timeout, ACE_INT64 iRangeStart, ACE_INT64 iRangeEnd, int iPieceID, bool bSupportHeadMethord, int iPieceNum, std::string filename);

	int connect ();
	virtual int svc(void);

	//suspend();
	//resume();

public:
	CHttpClient *m_pHttpClient;
	ACE_INT64 m_RangeStart;
	ACE_INT64 m_RangeEnd;

private:
	
	ACE_SOCK_Connector m_Connector;
	ACE_SOCK_Stream m_SockStream;
	ACE_Time_Value m_TimeOut;
	std::string m_strURL;
	int m_PieceID;
	int m_iPieceNum;
};

class CHttpDownloadTask: public ACE_Event_Handler
{
public:
	CHttpDownloadTask(std::string strURL, int iPiecesNum = 5);
	int LaunchTask();
	virtual int handle_timeout(const ACE_Time_Value & current_time, const void * = 0);
	virtual int handle_signal(int signum, siginfo_t * y=0,ucontext_t * i =0);
	bool GetSpeedAndRate(double &speed, int &rate);
	ACE_INT64 GetFileSize();
	bool Start();
	bool Resume();
	bool Suspend();
	bool Delete();
	bool Stop();
	~CHttpDownloadTask();

public:
	bool m_bTaskComplete;
	int m_TimerID;
	CFileStore m_FileStore;
	static bool m_bFirstDisplay;
	ACE_INT64 m_pre_offset;
	float m_fSpeed;
	int m_iPerc;
	ACE_INT64 m_FileSize;
	ACE_Time_Value m_StartTime;

//private:
	std::string m_strURL;
	std::string m_strFileName;
	int m_PieceNum;
	long m_TaskID;
	double time;
	std::map<int, CHttpConnector*> m_ThreadMap;
};

class CHttpDownloadTaskManager : public ACE_Event_Handler
{
public:
	CHttpDownloadTaskManager();
	int init ();
	long AddTask(std::string strURL, int iPiecesNum);
	bool StartTask(long lTaskID);
	bool DeleteTask(long lTaskID);
	bool SuspendTask(long lTaskID);
	bool ResumeTask(long lTaskID);
	bool StopTask(long lTaskID);
	bool GetSpeedAndRate(long lTaskID, double &speed, int &rate);
	ACE_INT64 GetFileSize(long lTaskID);
	CHttpDownloadTask* GetTask(long lTaskID);

protected:
	virtual int handle_timeout(const ACE_Time_Value & current_time, const void * = 0);

public:
	~CHttpDownloadTaskManager();

private:
	int m_iTaskID;
	std::map<long, CHttpDownloadTask*> m_TaskMap;
};





/*
class CTaskTypeParser
{
public:
	int GetTaskType();
class CFtpDownloadTask
{
public:
	CFtpDownloadTask(std::string strURL);
	int LaunchTask();

private:
	
};

class CFtpDownloadTaskManager : public ACE_Event_Handler
{

};


class CBTDownloadTask
{
public:
	CBTDownloadTask(std::string strURL);
	int LaunchTask();//

private:
	
};

class CBTDownloadTaskManager : public ACE_Event_Handler
{

};


class CResumeHttpDownloadTaskInfo
{
	
};

class CResumeFtpDownloadTaskInfo
{
	
};

class CResumeBTDownloadTaskInfo
{
	
};*/


#endif //_HTTP_DECODE_HEADER

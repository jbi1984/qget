#ifndef _FILESTORE_H
#define _FILESTORE_H

#include <iostream>
#include <map>
#include <algorithm>
#include "ace/SString.h"
#include "ace/FILE_IO.h"
#include "ace/Thread_Mutex.h"
#include "ace/Task_T.h"
#include "ace/SOCK_Connector.h"
#include "ace/Reactor.h"
#include "ace/FILE_Connector.h"
#include "ace/FILE_Addr.h"

struct PieceInfo
{
	unsigned int iRangeStart;
	unsigned int iRangeEnd;
	unsigned int iOffSet;
	int iPieceID;
	int iPieceNum;
	unsigned int iPieceLen;
	std::string strFileName;
	bool bComplete;
};


/*struct TaskInfo
{
	unsigned int iTaskNum;
	FileInfo *[iTaskNum];
};*/

//each thread has a CFileStore Object,no need mutex.
class CFileStore
{
public:
	struct  StoreInfo
	{
		std::string     name;
		ACE_FILE_IO     stream;
	};

   CFileStore();
	bool CreateDir(std::string strPath, unsigned int mode = ACE_DEFAULT_DIR_PERMS);
	static bool CreateFile(std::string strFile, ACE_INT64  filesize = 0);	
	bool Merge(const char* strFileName, int iPieceNum);

	int StoreData(std::string strFileName, ACE_INT64 offset, char* strData, int iDataLen);
	static int ReadTaskInfo(std::string strFile = "task.info");//can use ACE_FILE_Info
	static int ReadPieceInfo(std::string strPieceName);
	static int UpdateTaskInfo();
	static int UpdatePieceInfo(PieceInfo &tmpPieceInfo);

	//bool RmFile(std::string strFileName);
	//bool RmDir(std::string strDirName);

private:
	ACE_Thread_Mutex m_mutex;
	double m_percent;
	std::string m_filename;
	ACE_INT64 m_filesize;
};


#endif //_FILESTORE_H

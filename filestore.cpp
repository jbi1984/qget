/*
 * 2011 -2021 ,All rights reserved.
 * Contact: heguanbo@gmail.com/gjhe@novell.com
 */
#include "filestore.h"
#include "log.h"
#include <string>
#include <ace/Recursive_Thread_Mutex.h>
#include <ace/Mutex.h>

//static ACE_Thread_Mutex GFMutex;
//static ACE_Recursive_Thread_Mutex GFMutex;
static ACE_Mutex GFMutex;

CFileStore::CFileStore()
{
	//std::cout<<"Enter HttpFileStore::CFileStore!"<<std::endl;
}

bool CFileStore::CreateFile(std::string strFile, ACE_INT64 size)
{
	//std::cout<<"Enter HttpFileStore::CreateFile!"<<std::endl;
	//m_filename = strFile;
	//m_filesize = size;
	StoreInfo  info;
	info.name = strFile;
	ACE_FILE_Connector con;
	if (-1 == con.connect(info.stream, ACE_FILE_Addr(info.name.c_str())))	
	{
		QLOG("Can not create the file,please check the write file right, filename is %s\n", info.name.c_str());
		exit(1);
		return false;
	}

   	int i = info.stream.truncate(size);
	//std::cout<<"size of (ACE_OFF_T) "<<sizeof (ACE_OFF_T)<<"File size in create: "<<size<<std::endl;
	if (i != 0)
		QLOG("Set File Size to %Q failed,the return value is %d\n", size,i);
	info.stream.close();
    
   return true;
}

bool CFileStore::CreateDir(std::string strPath, unsigned int mode)
{
	//std::cout<<"Enter HttpFileStore::CreateDir!"<<std::endl;
	/*int x = ACE_OS::mkdir(strPath.c_str());
	if (-1 == x)
		return false;*/
 
	return true;
}

bool CFileStore::Merge(const char* strFileName, int iPieceNum)
{
	std::string strFile = strFileName;
	unsigned int ioffset = 0;
	//std::cout<<"FileName: "<<strFile<<std::endl;
	
	char buf[20];
	memset(buf, 0, sizeof(buf));
	char* pRead = NULL;
	char* pLastRead = NULL;
	int iReadSize = 1024*4;
	int iLastReadSize = 0;
	pRead = new char[iReadSize +1 ];
	if (pRead == NULL)
		std::cout<<"alloc mem failed!"<<std::endl;
	memset(pRead, 0, iReadSize +1);

	ACE_FILE_Info FileInfo;
	ACE_FILE_Connector con;
	StoreInfo  info;
	unsigned int itmpoffset = 0;
	for (int i=0;i<iPieceNum;++i)
	{
		info.name = strFile + ACE_OS::itoa(i, buf, 10);
CC:		if (0 != con.connect(info.stream, ACE_FILE_Addr(info.name.c_str())))
		{
			std::cout<<"connect failed----------------------------------->"<<info.name<<std::endl;
			goto CC;
	    	}

		info.stream.get_info(FileInfo);
		unsigned int iFileSize = FileInfo.size_;
		itmpoffset = 0;
		for (int i=0;i<iFileSize/iReadSize;++i)
		{
			memset(pRead, 0, iReadSize);
			info.stream.seek(itmpoffset, SEEK_SET);
			info.stream.recv_n(pRead, iReadSize);	
			StoreData(strFile, ioffset, pRead, iReadSize);
			ioffset += iReadSize;
			itmpoffset += iReadSize;
		}
		iLastReadSize = iFileSize % iReadSize;
		if (0 != iLastReadSize)
		{
			pLastRead = new char [iLastReadSize +1];
			if (NULL == pLastRead)
				std::cout<<"alloc mem failed!"<<std::endl;
			memset(pLastRead, 0, iLastReadSize +1);
			info.stream.seek(itmpoffset, SEEK_SET);
			info.stream.recv_n(pLastRead, iLastReadSize);
			StoreData(strFile, ioffset, pLastRead, iLastReadSize);
			ioffset += iLastReadSize;
			if (NULL != pLastRead)
			{
				delete []pLastRead;
				pLastRead = NULL;
			}
		}
		//std::cout<<"last Read ok"<<std::endl;
		info.stream.close();
		while(0 != remove(info.name.c_str()))
			;
	}

	delete []pRead;
	pRead = NULL;
	//std::cout<<"Out HttpFileStore::Merge!"<<std::endl;
	return true;
}

int CFileStore::ReadTaskInfo(std::string filename)
{
	std::cout<<"Enter HttpFileStore::ReadTaskInfo!"<<std::endl;
	StoreInfo  info;
	info.name = filename;
	ACE_FILE_Connector con;
	if (-1 == con.connect(info.stream, ACE_FILE_Addr(info.name.c_str())))
	{
		return -1;
    }

	ACE_FILE_Info FileInfo;
	info.stream.get_info(FileInfo);
	unsigned int iFileSize = FileInfo.size_;
	char buf[1024] = {0};
	info.stream.recv_n(buf, sizeof(buf));

	info.stream.close();

	return 0;
}

int CFileStore::ReadPieceInfo(std::string strPieceName)
{
	std::cout<<"Enter HttpFileStore::ReadPieceInfo!"<<std::endl;
	StoreInfo  info;
	info.name = strPieceName;
	ACE_FILE_Connector con;
	if (-1 == con.connect(info.stream, ACE_FILE_Addr(info.name.c_str())))
	{
		return -1;
    }

	ACE_FILE_Info FileInfo;
	info.stream.get_info(FileInfo);
	unsigned int iFileSize = FileInfo.size_;
	char buf[1024] = {0};
	info.stream.recv_n(buf, sizeof(buf));

	info.stream.close();

	return 0;
}

int CFileStore::StoreData(std::string strFileName, ACE_INT64 offset, char* strData, int iDataLen)
//int StoreData(std::string strFileName, unsigned int offset, std::string strData)
{
	//std::cout<<"Enter HttpFileStore::StoreData!"<<std::endl;
	GFMutex.acquire();
	StoreInfo  info;
	info.name = strFileName;
	ACE_FILE_Addr file_addr(info.name.c_str());
	ACE_FILE_Connector con;
RR:	if (-1 == con.connect(info.stream,file_addr))
	{
		std::cout<<"connect failed in StoreData"<<std::endl;
		QLOG("connect failed in StoreData(), the file name is %s\n", info.name.c_str());
		goto RR;
		//return -1;
	}

	//Sets the file pointer as follows: 
	//o If <whence> is <SEEK_SET>, the pointer is set to <offset> bytes.
	//o If <whence> is <SEEK_CUR>, the pointer is set to its current location plus <offset>.
	//o If <whence> is <SEEK_END>, the pointer is set to the size of the file plus offset. 
	int x = info.stream.seek(offset, SEEK_SET);
	int y = info.stream.send_n(strData, iDataLen);
	info.stream.close();
	GFMutex.release();
	return y;
}

int CFileStore::UpdatePieceInfo(PieceInfo &tmpPieceInfo)
{
	std::cout<<"Enter HttpFileStore::UpdatePieceInfo!"<<std::endl;
	StoreInfo  info;
	info.name = tmpPieceInfo.strFileName;
	ACE_FILE_Connector con;
	if (-1 == con.connect(info.stream, ACE_FILE_Addr(info.name.c_str())))
	{
		return -1;
    }


	//Sets the file pointer as follows: 
	//o If <whence> is <SEEK_SET>, the pointer is set to <offset> bytes.
	//o If <whence> is <SEEK_CUR>, the pointer is set to its current location plus <offset>.
	//o If <whence> is <SEEK_END>, the pointer is set to the size of the file plus offset. 
	//int x = info.stream.seek(tmpPieceInfo.iOffSet, SEEK_SET);
	//int y = info.stream.send_n(strData.c_str(), strData.length());
	info.stream.close();

	return 0;
}

int CFileStore::UpdateTaskInfo()
{
	std::cout<<"Enter HttpFileStore::UpdateTaskInfo!"<<std::endl;
	//Sets the file pointer as follows: 
	//o If <whence> is <SEEK_SET>, the pointer is set to <offset> bytes.
	//o If <whence> is <SEEK_CUR>, the pointer is set to its current location plus <offset>.
	//o If <whence> is <SEEK_END>, the pointer is set to the size of the file plus offset. 
	//int x = info.stream.seek(offset, SEEK_SET);
	//int y = info.stream.send_n(strData.c_str(), strData.length());
	//info.stream.close();

	return 0;
}
//end CFileStore-----------------------------------------------

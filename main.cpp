#include "ftpdecode.h"
#include "httpdecode.h"
#include "log.h"
#include <string>

bool bQGET_DEBUG = false;

int main(int argc, char* argv[])
{
	std::string strUsage = "";
	if (2 > argc)
	{
		std::cout<<std::endl<<"==>Usage: "<<std::endl;
		std::cout<<"'qget complete-URL [pieceNum]'"<<std::endl<<"In Default,the pieceNum is 1"<<std::endl<<std::endl;
		bQGET_DEBUG = true;
		return 0;
	}

	
	std::string strURL = "";
	std::string strDebug = "";
	int piecenum = 0;

	for(int i=1;i<argc;i++)
	{
		strDebug = argv[i];
		std::transform(strDebug.begin(), strDebug.end(), strDebug.begin(), toupper);
		if (strDebug == std::string("-D") || strDebug == std::string("-DEBUG"))
		{
			bQGET_DEBUG = true;
			if(i == 1)/* qget -d url   or    qget -d url 10 */
			{
				if(argc == 2)
				{
					std::cout<<"Wrong parameters format,please check!"<<std::endl;
					exit(1);
				}
				if(argc == 3)
				{
					strURL = argv[i+1];
					piecenum = 1;
				}
				if(argc == 4)	
				{
					strURL = argv[i+1];
					piecenum = ACE_OS::atoi(argv[i+2]);
				}
				break;
			}
			if(i == 2)/* qget url -d */
			{
				if(argc == 4)
				{
					std::cout<<"Wrong parameters format,please check!"<<std::endl;
					exit(1);
				}
				if(argc == 3)
				{
					strURL = argv[i-1];
					piecenum = 1;
				}
				break;
			}
			if(i == 3)/* qget url 10 -d */
			{
				if(argc == 4)
				{
					strURL = argv[i-2];
					piecenum = ACE_OS::atoi(argv[i-1]);
				}
				break;
			}
		}
	}
	/* get url   or   qget url 10 */
	if(!bQGET_DEBUG)
	{
		if(argc == 2)
		{
			strURL = argv[1];
			piecenum = 1;
		}
		else
		{
			strURL = argv[1];
			piecenum = ACE_OS::atoi(argv[2]);
		}
	}

	std::string strTrueUrl = strURL;
	std::transform(strURL.begin(), strURL.end(), strURL.begin(), toupper);

	if (piecenum > 200)
		piecenum = 200;

	std::string strsub = strURL.substr(0,6);
	if (std::string("FTP://") == strsub)
	{	
		CFTPDownloadTaskManager OFTPTaskManager;
		OFTPTaskManager.AddTask(strTrueUrl, piecenum);//iPiecesNum);
		//5 ms
		ACE_Time_Value initDelay(0,500*1000);
		ACE_Time_Value interval(0,500*1000);
		long timerid = ACE_Reactor::instance()->schedule_timer(&OFTPTaskManager, 0, initDelay, interval);
		ACE_Reactor::instance()->run_reactor_event_loop();
	}
	strsub = strURL.substr(0,7);
	if (std::string("HTTP://") == strsub)
	{
		CHttpDownloadTaskManager OHttpTaskManager;
		OHttpTaskManager.AddTask(strTrueUrl, piecenum);//iPiecesNum);

		//5 ms
		ACE_Time_Value initDelay(0,500*1000);
		ACE_Time_Value interval(0,500*1000);
		long timerid = ACE_Reactor::instance()->schedule_timer(&OHttpTaskManager, 0, initDelay, interval);
		ACE_Reactor::instance()->run_reactor_event_loop();
	}
	else
	{
		std::cout<<"Wrong URL,please check!"<<std::endl;
		std::cout<<std::endl<<"==>Usage: "<<std::endl;
		std::cout<<"'qget complete-URL [pieceNum]'"<<std::endl<<"In Default,the pieceNum is 1"<<std::endl<<std::endl;
	}
	return 0;
}

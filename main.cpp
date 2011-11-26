//#include <algorithm>
#include "ftpdecode.h"
#include "httpdecode.h"
#include "log.h"
#include <string>


extern bool bQGET_DEBUG;
extern bool bDP_GOON;

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

	/*
	int ch;
	opterr=0;

	while((ch=getopt(argc,argv,"Cc:Dd:Nn:Uu"))!=-1)
	{
		switch(ch)
		{
			case  'c':
			case  'C':
				DP_GOON = true;
				break;
			case  'd':
			case  'D':
				QGET_DEBUG = true;
				break;
			case  'n':
			case  'N':
				printf("option n\n");
				piecenum = ACE::OS_atoi(optarg);
				break;
			case  'u':
			case  'U':
				strURL = optarg;
				break;
			default:
				printf("other  option:%c\n",ch);
				break;
		}
	}
	*/



	std::string strURL = argv[1];
	std::transform(strURL.begin(), strURL.end(), strURL.begin(), toupper);

	int piecenum = 0;
	if (argc>2)
	{
		piecenum = ACE_OS::atoi(argv[2]);
		/*if (piecenum>300)
			piecenum = 300;*/
		if (piecenum>20)
			piecenum = 20;
	}

	std::string strsub = strURL.substr(0,6);
	if (std::string("FTP://") == strsub)
	{	
		CFTPDownloadTaskManager OFTPTaskManager;
		OFTPTaskManager.AddTask(std::string(argv[1]), argc>2?piecenum:1);//iPiecesNum);
		//1 s
		//ACE_Time_Value initDelay(0,100*1000);
		//ACE_Time_Value interval(0,100*1000);
		//5 ms
		ACE_Time_Value initDelay(0,500*1000);
		ACE_Time_Value interval(0,500*1000);
		//1 s
		//ACE_Time_Value initDelay(1,0);
		//ACE_Time_Value interval(1,0);
		long timerid = ACE_Reactor::instance()->schedule_timer(&OFTPTaskManager, 0, initDelay, interval);
		ACE_Reactor::instance()->run_reactor_event_loop();
		
		//while(1)
		//	ACE_OS::sleep(9);
		//ACE_Reactor::instance()->cancel_timer(timerid);
	}
	strsub = strURL.substr(0,7);
	if (std::string("HTTP://") == strsub)
	{
		CHttpDownloadTaskManager OHttpTaskManager;
		OHttpTaskManager.AddTask(std::string(argv[1]), argc>2?piecenum:1);//iPiecesNum);

		//1 ms
		//ACE_Time_Value initDelay(0,100*1000);
		//ACE_Time_Value interval(0,100*1000);
		//5 ms
		ACE_Time_Value initDelay(0,500*1000);
		ACE_Time_Value interval(0,500*1000);
		//1 s
		//ACE_Time_Value initDelay(1,0);
		//ACE_Time_Value interval(1,0);
		long timerid = ACE_Reactor::instance()->schedule_timer(&OHttpTaskManager, 0, initDelay, interval);
		ACE_Reactor::instance()->run_reactor_event_loop();
	
		//while(1)
		//	ACE_OS::sleep(9);
		//ACE_Reactor::instance()->cancel_timer(timerid);
	}
	else
	{
		std::cout<<"Wrong URL,please check!"<<std::endl;
		std::cout<<std::endl<<"==>Usage: "<<std::endl;
		std::cout<<"'qget complete-URL [pieceNum]'"<<std::endl<<"In Default,the pieceNum is 3"<<std::endl<<std::endl;
	}
	return 0;
}

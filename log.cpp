#include "log.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/streams.h"
#include "ace/Thread_Mutex.h"
#include "ace/Process_Mutex.h"
#include <fstream>
#include <string>
#include "ace/Log_Msg.h"
#include <ace/Recursive_Thread_Mutex.h>
#include <ace/Mutex.h>

extern bool bQGET_DEBUG;

static ACE_Mutex QLOGGMutex;

bool QLOG(const char*fmt,...)
{
	if(!bQGET_DEBUG)
		return true;
	QLOGGMutex.acquire();
	std::string strParameter = "";
	int d = 0,counter = 0;
	ACE_INT64 out_64 =0 ;
	char c, *s,*in = (char*)fmt,*conter = (char*)fmt;
	while (*conter != '\0')
	{	conter++;counter++;}
	char tmp[200];
	ACE_OS::memset(tmp,0,sizeof(tmp));
	ofstream myfile("qlog.log",std::ios::app);
	ACE_OSTREAM_TYPE * OUTPUT = &myfile;
	ACE_LOG_MSG->msg_ostream(OUTPUT,0);
	ACE_LOG_MSG->set_flags(ACE_Log_Msg::OSTREAM);//|ACE_Log_Msg::VERBOSE);
	ACE_LOG_MSG->clr_flags(ACE_Log_Msg::STDERR);
	std::string strString = "";
	ACE_DEBUG((LM_INFO, ACE_TEXT("[%d]"), (int)ACE_OS::thr_self()));

	va_list   ap;
	va_start(ap, fmt);

	{
		while (*in != '\0')
		{	
			if (*in != '%') 
			{
				strString += *in;
				--counter;
				if (counter == 0)
				{
					ACE_DEBUG((LM_INFO, ACE_TEXT(strString.c_str())));
					break;
				}
				++in;
				continue;
			} 
			ACE_DEBUG((LM_INFO, ACE_TEXT(strString.c_str())));
			strString = "";

			++in;
			--counter;
			switch(*in) 
			{ 
				case 's':    
					//strParameter += "char*p1 =\"";     
					//strParameter += "\"";
					s = va_arg(ap, char *);
					for ( ; *s; s++) {
						strParameter += *s;
					}
					ACE_DEBUG((LM_INFO, ACE_TEXT("%s"),strParameter.c_str()));
					strParameter = "";
					//strParameter += ",";)
					//std::cout<<strParameter<<std::endl;
					break;
				case 'd':       
					//strParameter += "int i1=";
					d = va_arg(ap, int);
					ACE_DEBUG((LM_INFO, ACE_TEXT("%d"),d));
					//printf("int %d", d);
					/*ACE_OS::itoa(d, tmp, 10);
					  strParameter += tmp;
					  strParameter += ",";
					  std::cout<<":pp "<<strParameter<<std::endl;*/
					break;
				case 'c':
					//strParameter += "char c1=";     
					c = (char)va_arg(ap, int);
					ACE_DEBUG((LM_INFO, ACE_TEXT("%c"),c));
					/*printf("char %c \n", c);
					  strParameter += c;
					  strParameter += ",";
					  std::cout<<strParameter<<std::endl;*/
					break;
				case 'Q':
					out_64 = va_arg(ap, ACE_INT64);
					ACE_DEBUG((LM_INFO, ACE_TEXT("%Q"),out_64));
					break;
				default:
					break;
			}
			if (counter > 0)
			{	--counter;++in;}
		}

		/*strParameter[strParameter.length()-1] = 0;

		  sprintf(tmp,fmt, ap);

		  printf("intput string:  %s ", strParameter.c_str());

		  ACE_DEBUG((LM_INFO, ACE_TEXT(in),strParameter.c_str()));*/
		ACE_LOG_MSG->clr_flags(ACE_Log_Msg::OSTREAM);
		ACE_LOG_MSG->set_flags(ACE_Log_Msg::STDERR);
		va_end(ap);
	} 

	QLOGGMutex.release();

	return true;
}

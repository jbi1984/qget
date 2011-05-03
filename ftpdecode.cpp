/*
 * 2011 -2021 ,All rights reserved.
 * Contact: heguanbo@gmail.com/gjhe@novell.com
 */
#ifndef _FTP_DECODE_CPP
#define _FTP_DECODE_CPP
#include "ftpdecode.h"
#include <string>
#include "func.h"
#include <ace/Recursive_Thread_Mutex.h>
#include <ace/Mutex.h>

static std::string strChangeLine = "\r\n";
static std::string strUser = "USER anonymous" + strChangeLine;
static std::string strPassWord = "PASS heguanbo@@gamil.com" + strChangeLine;
//static ACE_Recursive_Thread_Mutex FTPGMutex;
static ACE_Mutex FTPGMutex;
static bool bREST = false;
static ACE_UINT32 uServerIp = 0;
static std::string strServerIp = "";
bool bFTP_DP_GOON = false;
int gFTP_Rate = 0;
static int Complete_Num = 0;

//判断收到的回应头是否完整
int JudgeTheMsgInfo(std::string strRes, std::string strFlagString)
{ 
	std::string strsub = ""; 
	std::string strNotEnd = strFlagString + "-";
	std::string strEnd = strFlagString;
	std::string::size_type Idx_1 = 0;
	std::string::size_type Idx_2 = 0;
	bool flag = false;
	while(std::string::npos != (Idx_2 = strRes.find("\r\n", Idx_1)))
	{
		strsub = strRes.substr(Idx_1,Idx_2-Idx_1);
		if (std::string::npos == strsub.find(strNotEnd.c_str()))//not find '220-'
		{
			if (std::string::npos == strsub.find(strEnd.c_str()))//not find '220-' or '220'
			{
				//std::cout<<"not find :"<<strNotEnd<<" or " <<strNotEnd<<std::endl;
				;//return 0;//some thing unknow
			}
			else
			{
				//std::cout<<"find :"<<strFlagString<<std::endl;
				return 1;//find '220'
			}
		}
		else // find '220-'
		{
			//std::cout<<"find :"<<strNotEnd<<std::endl;
			flag = false;
		}
		Idx_1 = Idx_2+2;
		if (Idx_1  >= strRes.length() && (!flag))//avoid getting out the array boundary
		{
			//std::cout<<"&&&&&&&&&&&&&&&&&&&&&&"<<std::endl;
			return 2;//need to get more data from the netlink
		}
	}
	if (!flag && Idx_1 < strRes.length())
		return 2;
}


//need to build a complete status machine
namespace gFtpResCode
{
/*       110 Restart marker reply.
            In this case the text is exact and not left to the
            particular implementation; it must read:
            MARK yyyy = mmmm
            where yyyy is User-process data stream marker, and mmmm
            server's equivalent marker.  (note the spaces between
            markers and "=".)
         119 Terminal not available, will try mailbox.
         120 Service ready in nnn minutes
         125 Data connection already open; transfer starting
         150 File status okay; about to open data connection.
         151 User not local; Will forward to <user>@@<host>.
         152 User Unknown; Mail will be forwarded by the operator.
         200 Command okay
         202 Command not implemented, superfluous at this site.
         211 System status, or system help reply
         212 Directory status
         213 File status
         214 Help message
            (on how to use the server or the meaning of a particular
            non-standard command.  This reply is useful only to the
            human user.)
         215 <scheme> is the preferred scheme.
         220 Service ready for new user
         221 Service closing TELNET connection
            (logged out if appropriate)
         225 Data connection open; no transfer in progress
         226 Closing data connection;
            requested file action successful (for example, file transfer
            or file abort.)
         227 Entering Passive Mode.  h1,h2,h3,h4,p1,p2
         230 User logged in, proceed
         250 Requested file action okay, completed.
         331 User name okay, need password
         332 Need account for login
         350 Requested file action pending further information
         354 Start mail input; end with <CR><LF>.<CR><LF>
         421 Service not available, closing TELNET connection.
            This may be a reply to any command if the service knows it
            must shut down.]
         425 Can't open data connection
         426 Connection closed; transfer aborted.
         450 Requested file action not taken:
            file unavailable (e.g. file busy)
         451 Requested action aborted: local error in processing
         452 Requested action not taken:
            insufficient storage space in system
         500 Syntax error, command unrecognized
            [This may include errors such as command line too long.]
         501 Syntax error in parameters or arguments
         502 Command not implemented
         503 Bad sequence of commands
         504 Command not implemented for that parameter
         530 Not logged in
         532 Need account for storing files
         550 Requested action not taken:
            file unavailable (e.g. file not found, no access)
         551 Requested action aborted: page type unknown
         552 Requested file action aborted:
            exceeded storage allocation (for current directory or
            dataset)
         553 Requested action not taken:
            file name not allowed*/


//1.1XX
std::string strRestartMarkerReply = "110";// Restart marker reply.
std::string strTransferStarting = "125";// Data connection already open; transfer starting
//2.2XX
std::string strCommandOk = "200";// Command okay
std::string strNoTransfer = "225";// Data connection open; no transfer in progress
std::string strClosingDataCon = "226";// Closing data connection;requested file action successful (for example, file transfer or file abort.)

std::string strEnterPassiveMode = "227";// Entering Passive Mode.  h1,h2,h3,h4,p1,p2 superfluous
//3.3XX

//4.4XX
std::string strServiceNotAvailable = "421";// Service not available, closing TELNET connection.
            //This may be a reply to any command if the service knows it
            //must shut down.]
std::string strCanNotOpenDataCon = "425";// Can't open data connection
std::string strConAborted= "426";// Connection closed; transfer aborted.
//5.5XX
std::string strSyntaxError = "500";// Syntax error, command unrecognized[This may include errors such as command line too long.]
std::string strSyntaxErrorInPar = "501";// Syntax error in parameters or arguments
std::string strNotImplemented202 = "202";// Command not implemented, superfluous at this site.
std::string strNotImplemented502 = "502";//Command not implemented
std::string strBadSeqOfCommands = "503";// Bad sequence of commands
std::string strNoSupportParameter = "504";// Command not implemented for that parameter
std::string strLoginedIn = "230";// User logged in, proceed
std::string strNotLoginIn = "530";// Not logged in
std::string strNeedPassWord = "331";// User name okay, need password
std::string strNeedAccountForLogin = "332";// Need account for login
std::string strNeedAccountForStore = "532";// Need account for storing files
std::string strAboutToOpenDataCon = "150";// File status okay; about to open data connection.
};

namespace gFtpReqMethord
{
	static std::string strUsr = "USER";
	static std::string strQuit = "QUIT";
	static std::string strPort = "PORT";
	static std::string strType = "TYPE";
	static std::string strMode = "MODE";
	static std::string strStru = "STRU";
	static std::string strRetr = "RETR";
	static std::string strStor = "STOR";
	static std::string strNoop = "NOOP";
};

namespace gFtpResCode
{
	static std::string strResCode110 = "110";
	static std::string strResCode120 = "120";
	static std::string strResCode125 = "125";
	static std::string strResCode150 = "150";

	static std::string strResCode200 = "200";
	static std::string strResCode202 = "202";
	static std::string strResCode211 = "211";
	static std::string strResCode212 = "212";
	static std::string strResCode213 = "213";
	static std::string strResCode214 = "214";
	static std::string strResCode215 = "215";
	static std::string strResCode220 = "220";
	static std::string strResCode221 = "221";
	static std::string strResCode225 = "225";
	static std::string strResCode226 = "226";
	static std::string strResCode227 = "227";
	static std::string strResCode230 = "230";
	static std::string strResCode250 = "250";
	static std::string strResCode257 = "257";
	
	static std::string strResCode331 = "331";
	static std::string strResCode332 = "332";
	static std::string strResCode350 = "350";
	
	static std::string strResCode421 = "421";
	static std::string strResCode425 = "425";
	static std::string strResCode426 = "426";
	static std::string strResCode450 = "450";
	static std::string strResCode451 = "451";
	static std::string strResCode452 = "452";

	static std::string strResCode500 = "500";
	static std::string strResCode501 = "501";
	static std::string strResCode502 = "502";
	static std::string strResCode503 = "503";
	static std::string strResCode504 = "504";
	static std::string strResCode530 = "530";
	static std::string strResCode550 = "550";
	static std::string strResCode551 = "551";
	static std::string strResCode552 = "552";
	static std::string strResCode553 = "553";
};



CFTPClient::CFTPClient(int iPieceID)
{
	m_PieceID = iPieceID;
	m_fPercent = 0.0;
	m_offset = 0;
}

CFTPClient::~CFTPClient()
{
}

//this func is ok
ACE_INT64 CFTPClient::GetFileSize(std::string strReqURL, std::string & strFileName, bool &bSupportREST)//need to process SIGPIPE,too,will be added later.
{
	char buf[1024*2] = {0};
	ACE_OS::memset(buf, 0, sizeof(buf));
	std::string::size_type idx_1 = 0;
	std::string::size_type idx_2 = 0;
	std::string strsub = "";
	bool bFoo = false;
	int intflag = 0;
	std::string strTmp = strReqURL;
	std::string::size_type idx = strTmp.rfind('/');
	strFileName = strTmp.substr(idx+1);
	//std::cout<<"FileName: "<<strFileName<<std::endl;
	QLOG("FileNameInGetFileSize: %s\n", strFileName.c_str());
	bool bNeedValidAccount = false;//not anonymous

	std::string::size_type idx1 = strTmp.find("://", 0);
	std::string::size_type idx2 = strTmp.find('/', idx1+3);
	std::string strAddr = strTmp.substr(idx1+3, idx2-(idx1+3));
	std::string strURI = strTmp.substr(idx2);

	std::string strSYST = "SYST" + strChangeLine;
	std::string strTypeI = "TYPE I" + strChangeLine;
	std::string strPASV = "PASV" + strChangeLine;
		
	std::string strSIZE = "SIZE " + strURI + strChangeLine;
	std::string strAbort = "ABOR " + strChangeLine;

	ACE_INET_Addr addr("21", strAddr.c_str());

re_get_ip:

	/* methord 1
	uServerIp = addr.get_ip_address();
	uServerIp = htonl(uServerIp);
	struct in_addr addr_tmp;
	memcpy(&addr_tmp, &uServerIp, 4);
	strServerIp = inet_ntoa(addr_tmp);
	*/

	// methord 2
	if(!bFTP_DP_GOON)
		strServerIp = addr.get_host_addr();

	// methord 3
	// try to get all the ip of one domain. DNS search.



	QLOG("Serverv ip %s \n", strServerIp.c_str());
	//std::cout<<"Serverv ip: "<<strServerIp<<std::endl;
	addr = ACE_INET_Addr("21", strServerIp.c_str());



	ACE_OS::memset(buf, 0, sizeof (buf));
	ACE_SOCK_Connector m_Connector;
	ACE_SOCK_Stream m_SockStream;
	ACE_Time_Value m_TimeOut(10,0);
	ACE_Time_Value TimeOutShort(5,0);
	std::string strRes = "";

	int iCount = 0;
recon:
	int i = m_Connector.connect(m_SockStream, addr, &m_TimeOut);
	if (-1 == i)
	{
		QLOG("connect failed for geteing file size.\n");
		addr = ACE_INET_Addr("21", strServerIp.c_str());
		iCount +=10;
		m_SockStream.close();
		ACE_OS::sleep(iCount);
		if (300 < iCount)
		{
			iCount = 0;
			goto re_get_ip;
		}
		goto recon;
	}
	QLOG("connect ok,waiting the welcome msg\n");
WELCOM:	
	if (0 < m_SockStream.recv(buf, sizeof(buf), &TimeOutShort))
	{
		strRes = buf;
		QLOG("%s\n", strRes.c_str());
		ACE_OS::memset(buf, 0, sizeof (buf));
		intflag = JudgeTheMsgInfo(strRes, std::string("220"));
		if (2 == intflag)
			goto WELCOM;
		intflag = JudgeTheMsgInfo(strRes, std::string("230"));
                if (2 == intflag)
                        goto WELCOM;
	}
	
GO_USER:
	i = m_SockStream.send_n(strUser.c_str(), strUser.length(), &m_TimeOut);
	if (i<0)
	{
		QLOG("send command USER failed for getting file size,will reconnect.\n");
		goto recon;

	}
REUSER: 
	if (0 < m_SockStream.recv(buf, sizeof(buf), &m_TimeOut))
	{
		strRes = buf;
		QLOG("USER RES: %s\n",strRes.c_str());
		ACE_OS::memset(buf, 0, sizeof(buf));
		strsub = strRes.substr(0,3);
		if (strsub == gFtpResCode::strResCode230)
			goto GO_SYST;
		else if (strsub == gFtpResCode::strResCode530 || strsub == gFtpResCode::strResCode500
			|| strsub == gFtpResCode::strResCode501 || strsub == gFtpResCode::strResCode421)
		{		
			{
				QLOG("USER: Please try another account: \n");	
				bNeedValidAccount = true;
				std::cout<<"Please input the correct UserName: ";
				std::cin >> strUser;
				strUser = "USER " + strUser + strChangeLine;
				//std::cout<<"please input the passwd: ";
				//std::cin>>strPassWord;
				//strPassWord = "PASS " + strPassWord + strChangeLine;
				//goto GO_USER;
			}
			m_SockStream.close();
			goto recon;
		}
		else if (strsub == gFtpResCode::strResCode331)//username ok,need passwd
		{
			intflag = JudgeTheMsgInfo(strRes, std::string("331"));
			if (2 == intflag)
			{
				goto REUSER;
			}
			if (bNeedValidAccount)
			{
				std::cout<<"please input the passwd: ";
				std::cin>>strPassWord;
				strPassWord = "PASS " + strPassWord + strChangeLine;
			}
		}
		else if (strsub == gFtpResCode::strResCode332)//invalid account,need user to input another account 
		{
			bNeedValidAccount = true;
			std::cout<<"Invalid account,Please input the correct UserName: ";
			std::cin >> strUser;
			strUser = "USER " + strUser + strChangeLine;
			goto GO_USER;
		}



	
		//std::cout<<"USER res: "<<buf<<std::endl;
		//QLOG("USER RES: %s\n", buf);
		//ACE_OS::memset(buf, 0, sizeof(buf));
	}
	else
	{
		++iCount;
		if (iCount > 5)
			goto recon;
		else
			goto REUSER;
	}

	iCount = 0;
	i = m_SockStream.send_n(strPassWord.c_str(), strPassWord.length(), &m_TimeOut);
	if (0 > i)
	{
		QLOG("send command PASSWORD failed for getting file size,will reconnect.\n");
		goto recon;
	}
PASSRES:
	if (0 < m_SockStream.recv(buf, sizeof(buf), &m_TimeOut))
	{
		QLOG("PASSWORD RES: %s\n", buf);
		strRes = buf;
		ACE_OS::memset(buf, 0, sizeof (buf));
		strsub = strRes.substr(0,3);
		if (strsub == gFtpResCode::strResCode530 || strsub == gFtpResCode::strResCode500 
			|| strsub == gFtpResCode::strResCode501 || strsub == gFtpResCode::strResCode421)
		{		
			{
				std::cout<<"Please try another account "<<std::endl;
				QLOG("Please try another account: \n");	
				bNeedValidAccount = true;
				std::cout<<"Please input the correct UserName: ";
				std::cin >> strUser;
				strUser = "USER " + strUser + strChangeLine;
				//std::cout<<"please input the passwd: ";
				//std::cin>>strPassWord;
				//std::cout<<"null password!"<<std::endl;std::cout.flush();
				//strPassWord = "PASS " + strPassWord + strChangeLine;
			}
			//std::cout<<"&&&&&close ok!"<<std::endl;
			//std::cout.flush();
			m_SockStream.close();
			//std::cout<<"close ok!"<<std::endl;
			std::cout.flush();
			m_SockStream.close();
			goto recon;
		}
		else if (strsub == gFtpResCode::strResCode332)//invalid account,need user to input another account 
		{
			bNeedValidAccount = true;
			std::cout<<"Please input the correct UserName: ";
			std::cin >> strUser;
			strUser = "USER "+strUser + strChangeLine;
			goto GO_USER;
		}
		intflag = JudgeTheMsgInfo(strRes, std::string("230"));
                if (2 == intflag)
                        goto PASSRES;

	}
	else
	{
		++iCount;
		if (iCount > 5)
			goto recon;
		else
			goto PASSRES;
	}

GO_SYST:
	iCount = 0;
	i = m_SockStream.send_n(strSYST.c_str(), strSYST.length(), &m_TimeOut);
	if (i<0)
	{
		QLOG("send command SYST failed for getting filesize,will reconnect.\n");
		goto recon;
	}
SYST:
	if (0 < m_SockStream.recv(buf, sizeof(buf), &m_TimeOut))
	{
		QLOG("SYST RES: %s\n", buf);
		strRes = buf;
		strRes = strRes.substr(0,3);
		ACE_OS::memset(buf, 0, sizeof(buf));
	}
	else
	{
		++iCount;
		if (iCount > 5)
			goto recon;
		else
			goto SYST;
	}

	iCount = 0;
	i = m_SockStream.send_n(strTypeI.c_str(), strTypeI.length(), &m_TimeOut);
	if (i<0)
	{
		QLOG("send command TYPEI failed for getting file size,will reconnect.\n");
		goto recon;
	}
TYPEI:	
	if (0 < m_SockStream.recv(buf, sizeof(buf), &m_TimeOut))
	{
		QLOG("TYPE I RES: %s\n", buf);
		ACE_OS::memset(buf, 0, sizeof(buf));
	}
	else
	{
		++iCount;
		if (iCount > 5)
			goto recon;
		else
			goto TYPEI;
	}
	iCount = 0;

	std::string strlen = "";
	i = m_SockStream.send_n(strSIZE.c_str(), strSIZE.length(), &m_TimeOut);
	if (i<0)
	{
		QLOG("send command SIZE failed for getting file size,will reconnect.\n");
		goto recon;
	}
FTPSIZE:	//need to enhance the logic to get file size
	if (0 < m_SockStream.recv(buf, sizeof(buf), &m_TimeOut))
	{
		QLOG("SIZE RES: %s\n", buf);
		strRes = buf;
		strRes = strRes.substr(0,3);
		std::string tmp = buf;
		std::string::size_type idx11 = tmp.find("\r\n");
		std::string::size_type idx22 = tmp.find(' ');
		strlen = tmp.substr(idx22+1, idx11-1-(idx22+1) + 1);
		//std::cout<<"FileSize: "<<strlen<<" Bytes"<<std::endl;
		QLOG("FileSizeLenStrGotFromServer: %s Bytes\n", strlen.c_str());
		std::cout.flush();
		ACE_OS::memset(buf, 0, sizeof (buf));
	}
	else
	{
		++iCount;
		if (iCount > 5)
			goto recon;
		else
			goto FTPSIZE;
	}

// need to go on testing the server supporting REST or not
	iCount = 0;
first_GO_PASV2:
	i=m_SockStream.send_n(strPASV.c_str(), strPASV.length(), &m_TimeOut);
first_PASV2:
	if (0 < m_SockStream.recv(buf, sizeof(buf), &m_TimeOut))
	{
		iCount +=3;
		strRes = buf;
		ACE_OS::memset(buf, 0, sizeof(buf));
		strsub = strRes.substr(0,3);
		if (strsub == gFtpResCode::strResCode227)
		{
			intflag = JudgeTheMsgInfo(strRes, std::string("227"));
			if (2 == intflag)
			{
				goto first_PASV2;
			}
		}
		else if (strsub == gFtpResCode::strResCode500 || strsub == gFtpResCode::strResCode501)
		{
			QLOG("wrong rescode for PASSIVE,will reconnect. the rescode is %s \n", strsub.c_str());
			m_SockStream.close();
			ACE_OS::sleep(iCount);
			iCount = 0;
			goto recon;
		}
		else if (strsub == gFtpResCode::strResCode502)//command not implemented
		{
			std::cout<<"Not support command : PASV, so exit the program"<<std::endl;
			QLOG("Not support command : PASV, so exit the program\n");
			m_SockStream.close();
			exit(1);
		}
		else if (strsub == gFtpResCode::strResCode421)
		{
			QLOG("wrong rescode for PASSIVE,will reconnect. the rescode is %s \n", strsub.c_str());
			m_SockStream.close();
			ACE_OS::sleep(iCount);
			iCount = 0;
			goto recon;
		}
		else if (strsub == gFtpResCode::strResCode530)
		{
			QLOG("wrong rescode for PASSIVE,will reconnect. the rescode is %s \n", strsub.c_str());
			m_SockStream.close();
			ACE_OS::sleep(iCount);
			iCount = 0;
			goto recon;
		}

		std::string::size_type idx3 = strRes.rfind(')');
		idx1 = strRes.rfind(',');
		idx2 = strRes.rfind(',', idx1-1);
		std::string str1 = strRes.substr(idx2+1, idx1-(idx2+1));
		std::string str2 = strRes.substr(idx1+1, idx3-(idx1+1));
		int tmp1 = ACE_OS::atoi(str1.c_str());
		int tmp2 = ACE_OS::atoi(str2.c_str());
		unsigned short uPort = (unsigned short)(256*tmp1+tmp2);
		std::string::size_type id1,id2,id3,id4,id5;
		id1= strRes.find('(');
		id2= strRes.find(',',id1);
		str1 = strRes.substr(id1+1,id2-(id1+1));
		id3 = strRes.find(',', id2+1);
		str2 = strRes.substr(id2+1, id3-(id2+1));
		id4 = strRes.find(',', id3+1);
		std::string str3 = strRes.substr(id3+1,id4-(id3+1));
		id5 = strRes.find(',', id4+1);
		std::string str4 = strRes.substr(id4+1, id5-(id4+1));
		std::string strDestHost = str1+'.'+str2+'.'+str3+'.'+str4;
		QLOG("get-filesize: DEST HOST: %s  | DEST PORT: %d\n", strDestHost.c_str(), uPort);
	}
	else
	{
		QLOG("did not rev response for command PASSIVE, will reconnect.\n");
		m_SockStream.close();
		ACE_OS::sleep(iCount);
		iCount = 0;
		goto recon;
	}

//------------------------------------REST----------------------
	std::string strREST = "";
	strREST = std::string("REST ") + ACE_INT64_ITOA(9, buf, 10) + strChangeLine;
	QLOG("get-filesize: before tranfer num: %Q \n", ACE_INT64(9));
	QLOG("get-filesize: REST: %s \n", strREST.c_str());
	ACE_OS::memset(buf, 0, sizeof(buf));

	m_SockStream.send_n(strREST.c_str(), strREST.length(), &m_TimeOut);
	if (0 < m_SockStream.recv(buf, sizeof(buf), &m_TimeOut))
	{
		strRes = buf;
		ACE_OS::memset(buf, 0, sizeof(buf));
		strsub = strRes.substr(0,3);
		if (strsub == gFtpResCode::strResCode350)
		{
			QLOG("server support REST.\n");
			//std::cout<<"server support REST"<<std::endl;
			bSupportREST = true;
		}
		else
		{
			QLOG("server not support REST.\n");
			//std::cout<<"server not support REST"<<std::endl;
			bSupportREST = false;
		}
	}


	m_SockStream.close();
	QLOG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	return  ACE_INT64_ATOI(strlen.c_str(),strlen.length());
}

int CFTPClient::SaveData(char *buf, ACE_INT64 uOffSet, int uDataLen)
{
	m_FileStore.StoreData(m_strFileName, uOffSet, buf, uDataLen);
	return 0;
}



CFTPConnector::~CFTPConnector()
{
	if (NULL != m_pFTPClient)
	{
		delete m_pFTPClient;
		m_pFTPClient = NULL;
	}
}

CFTPConnector::CFTPConnector(std::string strURL, ACE_Time_Value &timeout, ACE_INT64 uRangeStart, ACE_INT64 uPieceLen, int iPieceID, int iPieceNum)
{
	m_pFTPClient = new CFTPClient(iPieceID);
	m_TimeOut = timeout;
	m_PieceID = iPieceID;
	m_PieceNum = iPieceNum;
	m_RangeStart = uRangeStart;
	m_uPieceLen = uPieceLen;
	//std::cout<<"start pos: "<<m_RangeStart<<" Piece len: "<<m_uPieceLen<<std::endl;
	if (sizeof(ACE_INT64) == 8)
		QLOG("Start Pos: %Q , Piece Len: %Q \n",m_RangeStart,m_uPieceLen);
	if (sizeof(ACE_INT64) == 4)
		QLOG("Start Pos: %d , Piece Len: %d\n",m_RangeStart,m_uPieceLen);
	m_strURL = strURL;
	m_pFTPClient->m_PieceID = m_PieceID;
	m_pFTPClient->m_offset = uRangeStart;
	m_pFTPClient->m_RangeStart = uRangeStart;
	m_pFTPClient->m_RangeEnd = uRangeStart + uPieceLen - 1;
}

//need to make this func stronger.
int CFTPConnector::connect ()
{
	QLOG("FTP Connecting begining------->\n");
	//1.connect to server port 21;
	std::string strTmp = m_strURL;
	std::string::size_type idx1 = strTmp.find("://", 0);
	std::string::size_type idx2 = strTmp.find('/', idx1+3);
	std::string strAddr = strTmp.substr(idx1+3, idx2-(idx1+3));
	std::string strURI = strTmp.substr(idx2);

	//std::string strChangeLine = "\r\n";
	std::string strSYST = "SYST" + strChangeLine;
	std::string strTypeI = "TYPE I" + strChangeLine;
	std::string strPASV = "PASV" + strChangeLine;
	char buf[1024*4] = {0};
	ACE_OS::memset(buf, 0, sizeof(buf));
	std::string::size_type idx_1 = 0;
	std::string::size_type idx_2 = 0;
	std::string strsub = "";
	int intflag = 0;
	bool bNeedValidAccount = false;//not anonymous
		
	std::string port = "192,168,1,3,4,150";//ACE_OS:std::cout<<"res: "<<buf<<std::endl;:itoa(random(), buf, 10);
	std::string strPort = "PORT " + port + strChangeLine;
	std::string strMode = "MODE BLOCK"; 
	std::string strRETR = "RETR " + strURI + strChangeLine;
	std::string strAbort = "ABOR " + strChangeLine;
	std::string strBye = "QUIT" + strChangeLine;

	if (0 == strAddr.c_str())
		QLOG("Domain is NULL!\n");
	ACE_INET_Addr remote_addr = ACE_INET_Addr("21", strServerIp.c_str());
	ACE_INET_Addr remote_addr2(22, "127.0.0.0");
	//ACE_OS::memset(&remote_addr, 0, sizeof (remote_addr));
	//ACE_INET_Addr local_addr(1025);
	std::string strDestHost = "";
	unsigned short uPort = 0;

//-------------------------CONNECT------------------------------
	bool bf = false;//what this for?
	ACE_OS::memset(buf, 0, sizeof (buf));
	std::string strRes = "";
	ACE_INT64 uTmp = 0;
	ACE_INT64 uLen = 0;
	ACE_INT64 iiitmp = 0;
	int iCount = 0;
	int i = -1;
	ACE_Time_Value con_timeout(20,0);

recondatatelnet:
	//iCount = 0;
	ACE_OS::memset(buf, 0, sizeof (buf));
	i = m_Connector.connect(m_SockStream, remote_addr, &con_timeout);
	if (0 != i)
	{
		if (iCount > 300)
		{
			std::cout<<"connect error,please check your internet connection!"<<std::endl;
			QLOG("connect error,please check your internet connection!\n");
			iCount = 0;
		}
		iCount += 5;
		m_SockStream.close();
		ACE_OS::sleep(iCount);
		remote_addr = ACE_INET_Addr("21", strServerIp.c_str());
		QLOG("************************************\nReConnecting...\n***************************************\n");
		goto recondatatelnet;
	}

//-------------------------WELCOME MESSAGE----------------------

	int iOnlyOneLine = 0;
	bool bEndOfWelcom = false;
	iCount = 0;
WELCOM2:
	if (0 < m_SockStream.recv(buf, sizeof(buf), &con_timeout))
	{
		iCount +=5;
		strRes = buf;
		ACE_OS::memset(buf, 0, sizeof (buf));
		//QLOG("The welcom msg: \n");
		//QLOG("%s \n", buf);
		strsub = "";
		strsub = strRes.substr(0,3);
		if (strsub == gFtpResCode::strResCode120 || strsub == gFtpResCode::strResCode421)
		{
			if(iCount > 10)
				QLOG("Wrong welcom ResCode,will reconnect.\n");
			m_SockStream.close();
			ACE_OS::sleep(iCount);
			iCount = 0;
			goto recondatatelnet;
		}
		else if (strsub == gFtpResCode::strResCode220)
		{
			intflag = JudgeTheMsgInfo(strRes, std::string("220"));
			if (2 == intflag)
			{
				goto WELCOM2;
			}
		}
	}
	else
	{
		iCount +=2;
		if (iCount > 10)
		{
			QLOG("Did not rev welcom msg ,will reconnect.\n");
			m_SockStream.close();
			ACE_OS::sleep(iCount);
			iCount = 0;
			goto recondatatelnet;
		}
		QLOG("Not rev welcom msg yet,will retry get welcom msg.\n");
		goto WELCOM2;
	}

//-------------------------USER---------------------------------
GO_USER2:
	//std::cout<<"USER: "<<std::endl;
	//QLOG("USER: \n");
	iCount = 0;
	iOnlyOneLine = 0;
	ACE_OS::memset(buf, 0, sizeof(buf));
	i=m_SockStream.send_n(strUser.c_str(), strUser.length(), &m_TimeOut);
	if (i<0)
	{
		QLOG("send command USER failed, will reconnect.\n");
		m_SockStream.close();
		goto recondatatelnet;
	}
USER2:
	if (0 < m_SockStream.recv(buf, sizeof(buf), &m_TimeOut))
	{	
		iCount +=3;		
		strRes = buf;
		//QLOG("%s\n",strRes.c_str());
		ACE_OS::memset(buf, 0, sizeof(buf));
		strsub = strRes.substr(0,3);
		if (strsub == gFtpResCode::strResCode230)
			goto GO_SYST2;
		else if (strsub == gFtpResCode::strResCode530 || strsub == gFtpResCode::strResCode500
			|| strsub == gFtpResCode::strResCode501 || strsub == gFtpResCode::strResCode421)
		{
			QLOG("Wrong USER ResCode,will reconnect.\n");
			m_SockStream.close();
			ACE_OS::sleep(iCount);
			iCount = 0;
			goto recondatatelnet;
		}
		else if (strsub == gFtpResCode::strResCode331)//username ok,need passwd
		{
			intflag = JudgeTheMsgInfo(strRes, std::string("331"));
			if (2 == intflag)
			{
				goto USER2;
			}
			/*if (bNeedValidAccount)
			{
				std::cout<<"please input the passwd: ";
				std::cin>>strPassWord;
				strPassWord = "PASS" + strPassWord + strChangeLine;
			}*/
		}
		else if (strsub == gFtpResCode::strResCode332)//invalid account,need user to input another account 
		{
			bNeedValidAccount = true;
			std::cout<<"Please input the UserName: ";
			std::cin >> strUser;
			strUser = "USER" + strUser + strChangeLine;
			goto GO_USER2;
		}
	}
	else
	{
		iCount +=2;
		if (iCount > 6)
		{	
			QLOG("No res to command USER, will reconnect.\n");
			iCount = 0;
			m_SockStream.close();
			goto recondatatelnet;
		}
		QLOG("No res to command USER yet, will retry.\n");
		goto USER2;
	}


//--------------------------PASSWORD----------------------------
	//std::cout<<"PASSWORD: "<<std::endl;
	//QLOG("PASSWORD: \n");
	iCount = 0;
	iOnlyOneLine = 0;
	i=m_SockStream.send_n(strPassWord.c_str(), strPassWord.length(), &m_TimeOut);
	if (i<0)
	{
		QLOG("send command PASSWORD failed, will reconnect.\n");
		m_SockStream.close();
		goto recondatatelnet;
	}
PASSRES2:
	if (0 < m_SockStream.recv(buf, sizeof(buf), &m_TimeOut))
	{
		iCount +=3;
		strRes = buf;
		//QLOG("%s\n", strRes.c_str());
		ACE_OS::memset(buf, 0, sizeof(buf));
		strsub = strRes.substr(0,3);
		if (strsub == gFtpResCode::strResCode230)
		{
			intflag = JudgeTheMsgInfo(strRes, std::string("230"));
			if (2 == intflag)
			{
				goto PASSRES2;
			}
		}
		else if (strsub == gFtpResCode::strResCode202)
		{
			QLOG("Wrong PASS ResCode,will exit!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!.\n");
			exit(0);
		}
		else if (strsub == gFtpResCode::strResCode530|| strsub == gFtpResCode::strResCode500 || strsub == gFtpResCode::strResCode501 
				|| strsub == gFtpResCode::strResCode503)
		{
			QLOG("Wrong PASS ResCode,will reconnect.the res is %s \n", strRes.c_str());
			m_SockStream.close();
			ACE_OS::sleep(iCount);
			iCount = 0;
			goto recondatatelnet;
		}
		else if (strsub == gFtpResCode::strResCode332)//need account(example,wrong passwd)
		{	
			QLOG("need account(maybe the passwd is wrong), will reconnect.\n");
			iCount = 0;
			m_SockStream.close();
			goto recondatatelnet;
		}
	}
	else
	{
		//std::cout<<"No res to PASSWD, try to continue receive PASSWD res msg!"<<std::endl;
		iCount +=2;
		if (iCount > 6)
		{
			QLOG("did not rev response for command PASSWORD, will reconnect.\n");
			m_SockStream.close();
			ACE_OS::sleep(iCount);
			iCount = 0;
			goto recondatatelnet;
		}
		QLOG("did not rev response for command PASSWORD yet, will retry.\n");
		goto PASSRES2;
	}
		
//-----------------------------SYST-----------------------------
GO_SYST2:
	//std::cout<<"SYST: "<<std::endl;
	//QLOG("SYST: \n");
	iCount = 0;
	i=m_SockStream.send_n(strSYST.c_str(), strSYST.length(), &m_TimeOut);
	if (i<0)
	{
		QLOG("send command SYST failed, will reconnect.\n");
		m_SockStream.close();
		goto recondatatelnet;
	}

SYST2:
	if (0 < m_SockStream.recv(buf, sizeof(buf), &m_TimeOut))
	{
		iCount +=3;
		strRes = buf;
		//QLOG("%s\n", strRes.c_str());
		ACE_OS::memset(buf, 0, sizeof(buf));
		strsub = strRes.substr(0,3);
		if (strsub == gFtpResCode::strResCode215)
		{
			intflag = JudgeTheMsgInfo(strRes, std::string("215"));
			if (2 == intflag)
			{
				goto SYST2;
			}
		}
		else if (strsub == gFtpResCode::strResCode500 || strsub == gFtpResCode::strResCode501 
			|| strsub == gFtpResCode::strResCode502) 
		{
			
			//why continue?
			QLOG("****************************\nWrong SYST ResCode,continue.\n**********************************\n");
		}
		else if(strsub == gFtpResCode::strResCode421)
		{
			QLOG("Wrong SYST ResCode,will reconnect.\n");
			m_SockStream.close();
			ACE_OS::sleep(iCount);
			iCount = 0;
			goto recondatatelnet;
		}
	}
	else
	{
		//std::cout<<"No res to SYST, try to continue receive SYST res msg!"<<std::endl;
		iCount +=2;
		if (iCount > 6)
		{
			QLOG("did not rev response for command SYST, will reconnect.\n");
			m_SockStream.close();
			ACE_OS::sleep(iCount);
			iCount = 0;
			goto recondatatelnet;
		}
		QLOG("did not rev response for command SYST yet, will retry.\n");
		goto SYST2;
	}
		
//--------------------------TYPE I------------------------------
	//std::cout<<"TYPE I: "<<std::endl;
	//QLOG("TYPE I: \n");
GO_TYPEI2:
	iCount = 0;
	i=m_SockStream.send_n(strTypeI.c_str(), strTypeI.length(), &m_TimeOut);
	if (i<0)
	{
		QLOG("send command TYPE I failed, will connect.\n");
		m_SockStream.close();
		goto recondatatelnet;
	}

TYPEI2:	
	if (0 < m_SockStream.recv(buf, sizeof(buf), &m_TimeOut))
	{
		iCount +=3;
		strRes = buf;
		//QLOG("%s\n", strRes.c_str());
		ACE_OS::memset(buf, 0, sizeof(buf));
		strsub = strRes.substr(0,3);
		if (strsub == gFtpResCode::strResCode200)
		{
			intflag = JudgeTheMsgInfo(strRes, std::string("200"));
			if (2 == intflag)
			{
				goto TYPEI2;
			}
		}
		else if (strsub == gFtpResCode::strResCode500 || strsub == gFtpResCode::strResCode501 
				|| strsub == gFtpResCode::strResCode504)
		{
			//why continue?
			//QLOG("Wrong TYPEI ResCode,continue.\n");
			QLOG("Wrong TYPEI ResCode,will reconnect. the rescode is %s \n", strsub.c_str());
			if (1 /*iCount > 10*/)
			{
				iCount =0;
				m_SockStream.close();
				goto recondatatelnet;
			}
			goto  GO_TYPEI2;
		}
		else if (strsub == gFtpResCode::strResCode421) 
		{
			//QLOG("Wrong TYPEI ResCode,will reconnect.\n");
			QLOG("Wrong TYPEI ResCode,will reconnect. the rescode is %s \n", strsub.c_str());
			m_SockStream.close();
			ACE_OS::sleep(iCount);
			iCount = 0;
			goto recondatatelnet;
		}
		else if (strsub == gFtpResCode::strResCode530)
		{
			QLOG("Not login,continue try to login.so reconnect.\n");
			{
				m_SockStream.close();
				ACE_OS::sleep(iCount);
				iCount = 0;
				goto recondatatelnet;
			}
		}
	}
	else
	{
		//std::cout<<"No res to TYPEI, try to continue receive TYPEI res msg!"<<std::endl;
		iCount +=2;
		if (iCount > 6)
		{
			QLOG("did not rev response for command TYPEI , reconnect.\n");
			m_SockStream.close();
			ACE_OS::sleep(iCount);
			iCount = 0;
			goto recondatatelnet;
		}
		QLOG("did not rev response for command TYPEI yet, retry.\n");
		goto TYPEI2;
	}

//-------------------------PASSIVE------------------------------
	iCount = 0;
GO_PASV2:
	//QLOG("PASSIVE: \n");
	i=m_SockStream.send_n(strPASV.c_str(), strPASV.length(), &m_TimeOut);
	if (i<0)
	{
		QLOG("send command PASSIVE failed, will reconnect.\n");
		m_SockStream.close();
		goto recondatatelnet;
	}

PASV2:	
	if (0 < m_SockStream.recv(buf, sizeof(buf), &m_TimeOut))
	{
		iCount +=3;
		strRes = buf;
		//QLOG("%s\n", strRes.c_str());
		ACE_OS::memset(buf, 0, sizeof(buf));
		strsub = strRes.substr(0,3);
		if (strsub == gFtpResCode::strResCode227)
		{
			intflag = JudgeTheMsgInfo(strRes, std::string("227"));
			if (2 == intflag)
			{
				goto PASV2;
			}
		}
		else if (strsub == gFtpResCode::strResCode500 || strsub == gFtpResCode::strResCode501)
		{
			//if (iCount > 10)
			//{
				QLOG("wrong rescode for PASSIVE,will reconnect. the rescode is %s \n", strsub.c_str());
				m_SockStream.close();
				ACE_OS::sleep(iCount);
				iCount = 0;
				goto recondatatelnet;
			//}
			//goto GO_PASV2;	
		}	
		else if (strsub == gFtpResCode::strResCode502)//command not implemented
		{
			std::cout<<"Not support command : PASV, so exit the program"<<std::endl;	
			QLOG("Not support command : PASV, so exit the program\n");
			m_SockStream.close();
			exit(1);
		}
		else if (strsub == gFtpResCode::strResCode421)
		{
			QLOG("wrong rescode for PASSIVE,will reconnect. the rescode is %s \n", strsub.c_str());
			m_SockStream.close();
			ACE_OS::sleep(iCount);
			iCount = 0;
			goto recondatatelnet;
		}
		else if (strsub == gFtpResCode::strResCode530)
		{	
			QLOG("wrong rescode for PASSIVE,will reconnect. the rescode is %s \n", strsub.c_str());
			m_SockStream.close();
			ACE_OS::sleep(iCount);
			iCount = 0;
			goto recondatatelnet;

		}

		std::string::size_type idx3 = strRes.rfind(')');
		idx1 = strRes.rfind(',');
		idx2 = strRes.rfind(',', idx1-1);
		std::string str1 = strRes.substr(idx2+1, idx1-(idx2+1));
		std::string str2 = strRes.substr(idx1+1, idx3-(idx1+1));
		int tmp1 = ACE_OS::atoi(str1.c_str());
		int tmp2 = ACE_OS::atoi(str2.c_str());
		uPort = (unsigned short)(256*tmp1+tmp2);
		//std::cout<<"*****************DEST PORT: "<<uPort<<std::endl;
		std::string::size_type id1,id2,id3,id4,id5;
		id1= strRes.find('(');
		id2= strRes.find(',',id1);
		str1 = strRes.substr(id1+1,id2-(id1+1));
		id3 = strRes.find(',', id2+1);
		str2 = strRes.substr(id2+1, id3-(id2+1));
		id4 = strRes.find(',', id3+1);
		std::string str3 = strRes.substr(id3+1,id4-(id3+1));
		id5 = strRes.find(',', id4+1);
		std::string str4 = strRes.substr(id4+1, id5-(id4+1));
		strDestHost = str1+'.'+str2+'.'+str3+'.'+str4;
		QLOG("DEST HOST: %s  | DEST PORT: %d\n", strDestHost.c_str(), uPort);
	}
	else
	{
		//std::cout<<"No res to PASSIVE, try to continue receive PASSIVE res msg!"<<std::endl;
		iCount +=2;
		if (iCount > 6)
		{
			QLOG("did not rev response for command PASSIVE, will reconnect.\n");
			m_SockStream.close();
			ACE_OS::sleep(iCount);
			iCount = 0;
			goto recondatatelnet;
		}
		QLOG("did not rev response for command PASSIVE yet, will retry.\n");
		goto PASV2;
	}
	
//----------------------data connect--------------------------
	iCount = 0;
	remote_addr2 = ACE_INET_Addr(uPort, strDestHost.c_str());
DATA_RECONNECT:
	if(0 !=  m_ConnectorData.connect(m_SockStreamData, remote_addr2, &m_TimeOut))
	{
		iCount +=2;
		if (iCount > 6)
		{
			QLOG("CFTPConnectorData: DataConnect error,will reconnect!\n");
			m_SockStreamData.close();
			m_SockStream.close();
			goto recondatatelnet;
		}
		QLOG("CFTPConnectorData: DataConnect error,will retry.\n");
		goto DATA_RECONNECT;
	}

//------------------------------------REST----------------------
	iCount = 0;
	if(bREST)
	{
		std::string strREST = "";
		strREST = std::string("REST ") + ACE_INT64_ITOA(m_RangeStart+uTmp, buf, 10) + strChangeLine;
		QLOG("before tranfer num: %Q \n", m_RangeStart+uTmp);
		QLOG("REST: %s \n", strREST.c_str());
		ACE_OS::memset(buf, 0, sizeof(buf));

		i=m_SockStream.send_n(strREST.c_str(), strREST.length(), &m_TimeOut);
		if (i<0)
		{
			QLOG("send command REST failed, will reconnect.\n");
			m_SockStream.close();
			goto recondatatelnet;
		}
REST2:	
		if (0 < m_SockStream.recv(buf, sizeof(buf), &m_TimeOut))
		{
			iCount +=5;
			strRes = buf;
			ACE_OS::memset(buf, 0, sizeof(buf));
			strsub = strRes.substr(0,3);
			if (strsub == gFtpResCode::strResCode350)
			{
				intflag = JudgeTheMsgInfo(strRes, std::string("350"));
				if (2 == intflag)
				{
					QLOG("not all res to REST received, will go on reciving.the res is : %s \n", strsub.c_str());
					goto REST2;
				}
			}
			else if (strsub == gFtpResCode::strResCode502)
			{
				if(m_PieceNum > 1)
				{ 
					std::cout<<"Not support command: REST , please adjust the piece number to 1 and retry!";	
					exit(1);
				}
			}
			else if (strsub == gFtpResCode::strResCode500 || strsub == gFtpResCode::strResCode501 
				||strsub == gFtpResCode::strResCode421 || strsub == gFtpResCode::strResCode530)
			{
				//QLOG("wrong rescode for command REST, will reconnect. the rescode is %s \n", strsub.c_str());
				QLOG("wrong rescode for command REST, will reconnect. the res is %s \n", strRes.c_str());
				m_SockStream.close();
				ACE_OS::sleep(30);
				iCount = 0;
				//issuse: from this point, if we reconnect, for one hostname can bind several ipaddress, and the different ipaddress's server maybe diff,
				//so,this is a issuse, maybe one ip's server support REST,another's server does not support REST.
				goto recondatatelnet;
			}
		}
		else
		{
			iCount +=2;
			if (iCount > 6)
			{
				QLOG("did not rev response for command REST, will reconnect.\n");
				m_SockStream.close();
				ACE_OS::sleep(iCount);
				iCount = 0;
				goto recondatatelnet;
			}
			QLOG("did not rev response for command REST yet, will retry.\n");
			goto REST2;
		}
	}


GO_RETR2:
	i=m_SockStream.send_n(strRETR.c_str(), strRETR.length(), &m_TimeOut);
	if (i<0)
	{
		QLOG("send command RETR failed, will reconnect.\n");
		m_SockStream.close();
		goto recondatatelnet;
	}

	
//-------------------------------RETR---------------------------
	//std::cout<<"RETR: "<<std::endl;
	//QLOG("RETR: \n");
	iCount = 0;
RETR2:	
	if (0 < m_SockStream.recv(buf, sizeof(buf), &m_TimeOut))
	{
		iCount +=3;
		strRes = buf;
		//QLOG("%s\n", strRes.c_str());
		ACE_OS::memset(buf, 0, sizeof(buf));
		if (strsub == gFtpResCode::strResCode125)
		{
			intflag = JudgeTheMsgInfo(strRes, std::string("125"));
			if (2 == intflag)
			{
				QLOG("125,will re_recv\n");
				goto RETR2;
			}
		}
		else if (strsub == gFtpResCode::strResCode150)
		{
			intflag = JudgeTheMsgInfo(strRes, std::string("150"));
			if (2 == intflag)
			{
				QLOG("150,will re_recv\n");
				goto RETR2;
			}
		}
		else if (strsub == gFtpResCode::strResCode250 || strsub == gFtpResCode::strResCode226)
		{
			QLOG("wrong rescode for command RETR,will reconnect.the rescode is %s \n", strsub.c_str());
			m_SockStreamData.close();
			m_SockStream.close();
			ACE_OS::sleep(iCount);
			iCount = 0;
			goto recondatatelnet;
		}
		else if (strsub == gFtpResCode::strResCode425)
		{
			QLOG("wrong rescode for command RETR,will reconnect.the rescode is %s \n", strsub.c_str());
			m_SockStreamData.close();
			m_SockStream.close();
			ACE_OS::sleep(iCount);
			iCount = 0;
			goto recondatatelnet;
		} 
		else if (strsub == gFtpResCode::strResCode450 || strsub == gFtpResCode::strResCode451
			|| strsub == gFtpResCode::strResCode426)
		{
			QLOG("wrong rescode for command RETR,will reconnect.the rescode is %s \n", strsub.c_str());
			m_SockStreamData.close();
			m_SockStream.close();
			ACE_OS::sleep(iCount);
			iCount = 0;
			goto recondatatelnet;
		}
		else if (strsub == gFtpResCode::strResCode421 || strsub == gFtpResCode::strResCode550
			|| strsub == gFtpResCode::strResCode500 || strsub == gFtpResCode::strResCode501
			|| strsub == gFtpResCode::strResCode530)
		{
			QLOG("wrong rescode for command RETR,will reconnect.the rescode is %s \n", strsub.c_str());
			m_SockStreamData.close();
			m_SockStream.close();
			ACE_OS::sleep(iCount);
			iCount = 0;
			goto recondatatelnet;
		}
	}
	else
	{
		iCount +=2;
		if (iCount > 6)
		{
			QLOG("did not rev response for command RETR, will re-connect!\n");
			iCount = 0;
			m_SockStreamData.close();
			m_SockStream.close();
			goto recondatatelnet;
		}
		QLOG("did not rev response for command RETR yet, will retry.\n");
		goto RETR2;
	}

//----------------------------DATA------------------------------
	iCount = 0;
	while (1)
	{
		uLen = 0;
		ACE_OS::memset(buf, 0, sizeof(buf));
GO_DATA:
		//pay attention to this, this may not stable,change to reconnect directly
		uLen = m_SockStreamData.recv(buf, sizeof(buf), &m_TimeOut);
		if (0 >= uLen )//|| uLen > sizeof(buf))
		{
			iCount +=5;
			QLOG("long time did not rev data,will reconnect!\n");
			ACE_OS::memset(buf, 0, sizeof(buf));
			m_SockStreamData.close();
			m_SockStream.close();
			ACE_OS::sleep(iCount);
			uLen = 0;
			iCount = 0;
			if(!bREST)
			{
				uTmp = 0;
				iiitmp = 0;
				uLen = 0;
			}
			goto recondatatelnet;
		}

		iiitmp = uTmp;
		uTmp += uLen;
		FTPGMutex.acquire();
		if (uTmp <= m_uPieceLen)
		{
			//FTPGMutex.acquire();
			m_pFTPClient->SaveData(buf, m_pFTPClient->m_offset, (int)uLen);
			m_pFTPClient->m_offset += uLen;
			m_pFTPClient->m_fPercent = ((float)uTmp)/(m_uPieceLen);
			if (uTmp == m_uPieceLen)
			{
				m_pFTPClient->m_IsThisPieceComplete = true;
				Complete_Num++;
				m_pFTPClient->m_fPercent = 1.0;
				QLOG("Piece %d complete!\n", m_PieceID);
				FTPGMutex.release();
				goto OVER;
			}
			//FTPGMutex.release();
		}
		else
		{
			if (m_PieceNum<=1)
				QLOG("Something strange occur,only 1 piece,should not go into this,please check*************** !\n");
			QLOG("rev overflow,****** Piece %d complete!\n", m_PieceID);
			QLOG("m_uPieceLen: %Q, uTmp: %Q , iiitmp: %Q \n", m_uPieceLen, uTmp, iiitmp);
			//FTPGMutex.acquire();
			//buf[m_uPieceLen - iiitmp]= '\0';
			m_pFTPClient->SaveData(buf, m_pFTPClient->m_offset, int(m_uPieceLen - iiitmp));
			m_pFTPClient->m_offset += m_uPieceLen-iiitmp;
			m_pFTPClient->m_fPercent = 1.0;
			m_pFTPClient->m_IsThisPieceComplete = true;
			Complete_Num++;
			FTPGMutex.release();
			goto OVER;
		}
		FTPGMutex.release();
	}		
		
OVER:
	//m_SockStream.send_n(strBye.c_str(), strBye.length(), &m_TimeOut);
	//m_SockStream.recv(buf, sizeof(buf), &m_TimeOut);
	m_SockStreamData.close();
	m_SockStream.close();

	return 0;
}

int CFTPConnector::svc(void)
{
	connect ();
	return 0;
}

bool CFTPDownloadTask::m_bFirstDisplay= false;
CFTPDownloadTask::CFTPDownloadTask(std::string strURL, unsigned int iPieceNum)
{
	m_bTaskComplete = false;
	m_TimerID = 0;;
	m_strURL = strURL;
	//std::string m_strFileName;
	m_PieceNum = iPieceNum;
	m_TaskID = 0;
	m_strURL = strURL;
	m_strFileName = "";
	//std::cout<<"FTPTask created!"<<std::endl;
	time = 0.0;
	m_pre_offset = 0;
	m_StartTime = ACE_OS::gettimeofday();
	m_FileSize = 0;
	ACE_Reactor::instance()->register_handler(SIGPIPE, this);

}
unsigned int CFTPDownloadTask::LaunchTask()//start threads
{
	//1.get the file size first , and get clear it the server support continue transfer mode(support REST)
	ACE_INT64 uFileSize = CFTPClient::GetFileSize(m_strURL, m_strFileName, bREST);
	if (!bREST)
	{
		QLOG("Server does not support REST, so we adjust the piece num to 1 \n");
		m_PieceNum = 1;
	}
	m_FileSize = uFileSize;

	std::string strLogFile = "."+m_strFileName;
	FILE *f = fopen(strLogFile.c_str(), "rb");
	if (f == NULL)
	{
		QLOG("Can not open file %s, file does not exist.So add a new Task.\n", strLogFile.c_str());
		bFTP_DP_GOON = false;
	}
	else
	{
		fclose(f);
		bFTP_DP_GOON = true;
	}


	if(bFTP_DP_GOON)
	{
		if (!bREST)
		{
			QLOG("The Now server does not support REST methord, so add a new task.");
			goto ftpnewtask;
		}
		QLOG("Continue downloading the file from dis-connect point:\n");

		std::cout<<"FileName: "<<m_strFileName<<std::endl;
		if (m_FileSize/1024/1024)
			std::cout<<"FileSize: "<<(m_FileSize/1024/1024)<<" MB"<<std::endl;
		else if(m_FileSize/1024)
			std::cout<<"FileSize: "<<(m_FileSize/1024)<<" KB"<<std::endl;
		else
			std::cout<<"FileSize: "<<(m_FileSize)<<" Bytes"<<std::endl;
		QLOG("FileSize: %Q\n", m_FileSize);


		//read DP_GOON datas from log file.
		int size = 0;
		f = fopen(strLogFile.c_str(), "rb");
		if (f == NULL)
		{
			QLOG("Can not open file %s, file does not exist.\n", strLogFile.c_str());
			return -1;                              
		}
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fseek(f, 0, SEEK_SET);
		char *result = (char*)malloc(size+1);
		if (size != fread(result, sizeof(char), size, f))
		{
			free(result);
			return -2;                              
		}
		fclose(f);
		result[size] = 0;

		std::string strReadLog = (char*)result;
		QLOG("Read log: %s",strReadLog.c_str());
		free(result);

		//get server ip
		std::string::size_type iPos1 = strReadLog.find('[', 0);
		std::string::size_type iPos2 = strReadLog.find(']', iPos1);
		strServerIp = strReadLog.substr(iPos1+1, iPos2 - iPos1 -1 );
		QLOG("ServerIp get from log : %s", strServerIp.c_str());

		//get the rate
		iPos1 = strReadLog.find('[', iPos2);
		iPos2 = strReadLog.find(']', iPos1);
		std::string strRate = strReadLog.substr(iPos1+1, iPos2 - iPos1 -1 );
		gFTP_Rate = ACE_INT64_ATOI(strRate.c_str(), strRate.length());

		//get range-start/range-end pairs number
		int i = 0;
		std::string::size_type iPos3 = 0;
		while(strReadLog.find('|', iPos3) != std::string::npos)
		{
			iPos3 = strReadLog.find('|', iPos3) + 1;
			i++;
		}

		struct RangePair
		{
			ACE_INT64 RangeStart;
			ACE_INT64 RangeEnd;
		};
		RangePair PairArray[i];
		std::string strRangePair = "";
		//get all range-start/range-end datas
		int j = i;
		while(i>0)
		{
			iPos1 = strReadLog.find('|', iPos2);
			iPos2 = strReadLog.find('|', iPos1 + 1);
			if (iPos2 == std::string::npos)
				strRangePair = strReadLog.substr(iPos1+1);
			else
				strRangePair = strReadLog.substr(iPos1+1, iPos2-iPos1-1);
			iPos2 = iPos1+1;

			iPos3 = strRangePair.find(':', 0);
			std::string strStart = strRangePair.substr(0, iPos3);
			std::string strEnd = strRangePair.substr(iPos3+1);
			PairArray[i-1].RangeStart = ACE_INT64_ATOI(strStart.c_str(), strStart.length());
			PairArray[i-1].RangeEnd = ACE_INT64_ATOI(strEnd.c_str(), strEnd.length());
			//std::cout<<PairArray[i-1].RangeStart<<":"<<PairArray[i-1].RangeEnd<<std::endl;
			--i;
		}

		QLOG("< Continue...... >\n");
		std::cout<<"< Continue...... >"<<std::endl;
		m_PieceNum = j;
		CFTPConnector* pConnector = NULL;
		ACE_Time_Value timeout(20,0);
		while(j>0)
		{
			pConnector = new CFTPConnector(m_strURL, timeout, \
					PairArray[j-1].RangeStart, PairArray[j-1].RangeEnd-PairArray[j-1].RangeStart+1, j-1, m_PieceNum);
			if (NULL != pConnector)
			{
				pConnector->m_pFTPClient->m_strFileName = m_strFileName;
				m_ThreadMap.insert(std::map<int, CFTPConnector*>::value_type(j-1, pConnector));
				while(-1 == pConnector->activate(THR_NEW_LWP|THR_JOINABLE|THR_INHERIT_SCHED|THR_SUSPENDED))
				{
					ACE_OS::sleep(1);
					std::cout<<"activate thread failed!maybe the thread number is too large!"<<std::endl;
				}
			}
			else
			{
				std::cout<<"alloc memory failed!"<<std::endl;
				exit(1);
			}
			--j;
		}

		Start();
		QLOG("HttpTask Launched!\n");

		return m_PieceNum;

	}



ftpnewtask:

	if (m_FileSize == 0)
		m_PieceNum = 1;
	else
		CFileStore::CreateFile(m_strFileName, m_FileSize);
	//2.start the piece threads
	ACE_INT64 iStartPos = (uFileSize)/(ACE_INT64)m_PieceNum;
	ACE_INT64 iLeft = (uFileSize)%(ACE_INT64)m_PieceNum;
	std::cout<<"FileName: "<<m_strFileName<<std::endl;


	if (m_FileSize/1024/1024)
		std::cout<<"FileSize: "<<(m_FileSize/1024/1024)<<" MB"<<std::endl;
	else if(m_FileSize/1024)
		std::cout<<"FileSize: "<<(m_FileSize/1024)<<" KB"<<std::endl;
	else
		std::cout<<"FileSize: "<<(m_FileSize)<<" Bytes"<<std::endl;
	QLOG("FileSize: %Q\n", m_FileSize);


	//std::cout<<"FileSize: "<<uFileSize<<std::endl;
	//std::cout<<"piecenum: | "<<m_PieceNum<<"pieceSize: "<<iStartPos<<std::endl;
	//std::cout<<"sizeof ACE_INT64 :  "<<sizeof(ACE_INT64)<<std::endl;
	CFTPConnector* pConnector = NULL;
	ACE_Time_Value timeout(20,0);
	int i=0;char tt[10];
	if (m_PieceNum > 1)
	{
		for (i=0;i < m_PieceNum-1;++i)
		{
			pConnector = new CFTPConnector(m_strURL, timeout, i*iStartPos, iStartPos, i, m_PieceNum);
			pConnector->m_pFTPClient->m_strFileName = m_strFileName;
			m_ThreadMap.insert(std::map<int, CFTPConnector*>::value_type(i, pConnector));
			while(-1 == pConnector->activate(THR_NEW_LWP|THR_JOINABLE|THR_INHERIT_SCHED|THR_SUSPENDED));
		}
		pConnector = new CFTPConnector(m_strURL, timeout, i*iStartPos, uFileSize-i*iStartPos, i, m_PieceNum);
		pConnector->m_pFTPClient->m_strFileName = m_strFileName;
		m_ThreadMap.insert(std::map<int, CFTPConnector*>::value_type(i, pConnector));
		while(-1 == pConnector->activate(THR_NEW_LWP|THR_JOINABLE|THR_INHERIT_SCHED|THR_SUSPENDED));
	}
	else
	{
		pConnector = new CFTPConnector(m_strURL, timeout, 0, uFileSize, 0, m_PieceNum);
		pConnector->m_pFTPClient->m_strFileName = m_strFileName;
		m_ThreadMap.insert(std::map<int, CFTPConnector*>::value_type(i, pConnector));
		while(-1 == pConnector->activate(THR_NEW_LWP|THR_JOINABLE|THR_INHERIT_SCHED|THR_SUSPENDED));
	}
	this->Start();
	return m_PieceNum;
}
int CFTPDownloadTask::handle_signal(int signum, siginfo_t*,ucontext_t *)
{
	QLOG("FTP SIGPIPE\n");
	QLOG("Suspend...\n");
	Suspend();
	ACE_OS::sleep(5);
	QLOG("Resume...\n");
	Resume();


	return 0;
}

//this is used for update speed, 
int CFTPDownloadTask::handle_timeout(const ACE_Time_Value & current_time, const void *)
{
	FTPGMutex.acquire();
	time+=0.1;
	std::map<int, CFTPConnector*>::iterator pos;
	float percent = 0;
	ACE_INT64 offset = 0;
	for (pos = m_ThreadMap.begin(); pos != m_ThreadMap.end(); ++pos)
	{
		percent += (pos->second)->m_pFTPClient->m_fPercent;
		//offset += (pos->second)->m_uPieceLen*(pos->second)->m_pFTPClient->m_fPercent/1024;
		//offset += (pos->second)->m_uPieceLen*(pos->second)->m_pFTPClient->m_fPercent/1024;
		offset += ((pos->second)->m_pFTPClient->m_offset - (pos->second)->m_pFTPClient->m_RangeStart) / 1024;
	}
	int perc = int((percent/m_PieceNum)*100);
	if (false == CFTPDownloadTask::m_bFirstDisplay)
	{
		std::cout<<std::setw(6)<<std::right<<"\r=> %"<<perc<<"|speed:"<<std::setw(10)<<std::right<<(offset-m_pre_offset)/(float)0.1  \
			<<"KB/S |" <<" arg speed: "<<std::setw(5)<<std::right<<std::setprecision(4)<<offset/(float)time<<"KB/S |"  \
			<<" time: "<<std::setw(6)<<std::right<<time<<" s";
		CFTPDownloadTask::m_bFirstDisplay= true;
		m_fSpeed = (offset-m_pre_offset)/(float)0.1;
		m_iPerc = perc;
	}
	else
	{
		std::cout<<std::setw(6)<<std::right<<"\r=> %"<<perc<<" | speed: "<<std::setw(10)<<std::right<<(offset-m_pre_offset)/(float)0.1 \
			<<"KB/S |" <<" arg speed: "<<std::setw(10)<<std::right<<std::setprecision(4)<<offset/(float)time<<"KB/S |" \
			<<" time: "<<std::setw(6)<<std::right<<time<<" s";
		m_fSpeed = (offset-m_pre_offset)/(float)0.1;
		m_iPerc = perc;
	}
	std::cout.flush();
	m_pre_offset = offset;

	bool bTmp =  false;
	for (pos = m_ThreadMap.begin(); pos != m_ThreadMap.end(); ++pos)
	{
		bTmp = (pos->second)->m_pFTPClient->m_IsThisPieceComplete;
		if (true == bTmp)
		{
		}
		else
		{
			break;
		}	
	}
	if(m_ThreadMap.end() == pos && m_PieceNum == m_ThreadMap.size())
	{
		m_bTaskComplete = true;	
	}

	FTPGMutex.release();
	return 1;
}

bool CFTPDownloadTask::GetSpeedAndRate(double &speed, int &rate)
{
	//std::cout<<"*************CFTP-->GetSpeedAndRate************"<<std::endl;
	FTPGMutex.acquire();
	ACE_Time_Value cur = ACE_OS::gettimeofday();
	ACE_Time_Value tmp = cur - m_StartTime;
	m_StartTime = cur;
	std::map<int, CFTPConnector*>::iterator pos;
	float percent = 0.001;
	ACE_INT64 offset = 0;

	for (pos = m_ThreadMap.begin(); pos != m_ThreadMap.end(); ++pos)
	{
		percent += (pos->second)->m_pFTPClient->m_fPercent;
		offset += (pos->second)->m_uPieceLen*(pos->second)->m_pFTPClient->m_fPercent/1024;
	}
	int perc = int((percent/m_PieceNum)*100);
	rate = perc;

	float sp = 0.001;
	sp = (offset-m_pre_offset)/((tmp.msec()/1000)>0?(tmp.msec()/1000):1 );
	speed = sp;
	m_pre_offset = offset;

	bool bTmp =  false;
	for (pos = m_ThreadMap.begin(); pos != m_ThreadMap.end(); ++pos)
	{
		bTmp = (pos->second)->m_pFTPClient->m_IsThisPieceComplete;
		if (true == bTmp)
		{
			;
		}
		else
		{
			break;
		}	
	}
	if(m_ThreadMap.end() == pos  && m_PieceNum == m_ThreadMap.size())
	{
		//m_FileStore.Merge(m_strFileName.c_str(), m_PieceNum);
		//std::cout<<"Merge "<<m_strFileName<<std::endl;
		//QLOG("Merge...\n");
		m_bTaskComplete = true;
	}
	FTPGMutex.release();

	return true;
}

bool CFTPDownloadTask::Start()
{
	//FTPGMutex.acquire();
	std::map<int, CFTPConnector*>::iterator pos;
	for (pos = m_ThreadMap.begin(); pos != m_ThreadMap.end();++pos)
	{
		pos->second->resume();
	}	
	//FTPGMutex.release();
	return true;
}
bool CFTPDownloadTask::Resume()
{
	std::map<int, CFTPConnector*>::iterator pos;
	for (pos = m_ThreadMap.begin(); pos != m_ThreadMap.end();++pos)
	{
		pos->second->resume();
	}	
	return true;
}
bool CFTPDownloadTask::Suspend()
{
	std::map<int, CFTPConnector*>::iterator pos;
	for (pos = m_ThreadMap.begin(); pos != m_ThreadMap.end();++pos)
	{
		pos->second->suspend();
	}	
	return true;
}
bool CFTPDownloadTask::Delete()
{
	return true;
}
bool CFTPDownloadTask::Stop()
{
	return true;
}
ACE_INT64 CFTPDownloadTask::GetFileSize()
{
	/*unsigned int uTmp = 0;
	  FTPGMutex.acquire();
	  uTmp = m_FileSize;
	  FTPGMutex.release();
	  return uTmp;*/
	return m_FileSize;
}

CFTPDownloadTask::~CFTPDownloadTask()
{
	//clear the thread map
	//FTPGMutex.acquire();
	std::map<int, CFTPConnector*>::iterator pos;
	for (pos = m_ThreadMap.begin(); pos != m_ThreadMap.end();++pos)
	{
		if(NULL != pos->second)
		{
			delete pos->second;
			pos->second = NULL;
		}
	}
	m_ThreadMap.clear();
	//FTPGMutex.release();
}
//end CFTPDownTask


//Begin CFTPDownloadTaskManager
CFTPDownloadTaskManager::CFTPDownloadTaskManager()
{
	m_iTaskID = 1024*4;
	m_TaskMap.clear();
	//std::cout<<"FTPTaskManager created!"<<std::endl;
}
//int init ();
long CFTPDownloadTaskManager::AddTask(std::string strURL, int iPiecesNum)
{
	CFTPDownloadTask* pFTPTask = new CFTPDownloadTask(strURL, iPiecesNum);
	//start the timer,need to remember the timerid to cancle it
	//this timer is used to update the speed
	//1 ms 
	//ACE_Time_Value initDelay(0, 100*1000);
	//ACE_Time_Value interval(0, 100*1000);
	//pFTPTask->m_TimerID = ACE_Reactor::instance()->schedule_timer(pFTPTask, 0, initDelay, interval);
	pFTPTask->LaunchTask();

	++m_iTaskID;
	pFTPTask->m_TaskID = m_iTaskID;
	m_TaskMap.insert(std::map<long, CFTPDownloadTask*>::value_type(m_iTaskID, pFTPTask));
	return m_iTaskID;
}

CFTPDownloadTask* CFTPDownloadTaskManager::GetTask(long lTaskID)
{
	std::map<long, CFTPDownloadTask*>::iterator pos;
	pos = m_TaskMap.find(lTaskID);

	return pos->second;
}
bool CFTPDownloadTaskManager::GetSpeedAndRate(long lTaskID, double &speed, int &rate)
{
	CFTPDownloadTask* p = GetTask(lTaskID);
	p->GetSpeedAndRate(speed, rate);
	return true;
}
ACE_INT64 CFTPDownloadTaskManager::GetFileSize(long lTaskID)
{
	CFTPDownloadTask* p = GetTask(lTaskID);
	return p->GetFileSize();
}

bool CFTPDownloadTaskManager::StartTask(long lTaskID)
{
	CFTPDownloadTask* p = GetTask(lTaskID);
	p->Start();
	return true;
}
bool CFTPDownloadTaskManager::DeleteTask(long lTaskID)
{
	//CFTPDownloadTask* p = GetTask(lTaskID);
	//p->Delete();
	return true;
}
bool CFTPDownloadTaskManager::SuspendTask(long lTaskID)
{
	CFTPDownloadTask* p = GetTask(lTaskID);
	p->Suspend();
	return true;
}
bool CFTPDownloadTaskManager::ResumeTask(long lTaskID)
{
	CFTPDownloadTask* p = GetTask(lTaskID);
	p->Resume();
	return true;
}
bool CFTPDownloadTaskManager::StopTask(long lTaskID)
{
	CFTPDownloadTask* p = GetTask(lTaskID);
	p->Stop();
	return true;
}

//this is used to check if the task is completed.
//why 2 timers? 1 timer will be better
int CFTPDownloadTaskManager::handle_timeout(const ACE_Time_Value & current_time, const void *)
{
	//disconnect-point log
	std::string strDP_Log = "";

	float t_piece = 0.5;//5 ms

	//FTPGMutex.acquire();
	CFTPConnector * pCon = NULL;
	std::map<long, CFTPDownloadTask*>::iterator pos;
	for (pos = m_TaskMap.begin(); pos != m_TaskMap.end(); pos++)
	{
		if (NULL != pos->second)
		{
			FTPGMutex.acquire();
			//actually, we should update speed here. and use only 1 timer
			pos->second->time += t_piece;
			std::map<int, CFTPConnector*>::iterator pos2;
			float percent = 0.0;
			unsigned int offset = 0;
			ACE_INT64 iPartFileSize = 0;
			int perc = 0;
			for (pos2 = pos->second->m_ThreadMap.begin(); pos2 != pos->second->m_ThreadMap.end(); ++pos2)
			{
				if (!pos2->second->m_pFTPClient->m_IsThisPieceComplete)
				{
					strDP_Log += "|" + ACE_INT64_ITOA(pos2->second->m_pFTPClient->m_offset, 10) + ":" \
						      + ACE_INT64_ITOA(pos2->second->m_pFTPClient->m_RangeEnd, 10);
				}


				//percent += (pos2->second)->m_pFTPClient->m_fPercent;
				if(bFTP_DP_GOON)
				{
					iPartFileSize += pos2->second->m_pFTPClient->m_fPercent * \
							 (pos2->second->m_pFTPClient->m_RangeEnd-pos2->second->m_pFTPClient->m_RangeStart+1);
				}
				else
				{
					percent += (pos2->second)->m_pFTPClient->m_fPercent;
				}
				offset += ((pos2->second->m_pFTPClient->m_offset - pos2->second->m_pFTPClient->m_RangeStart) / 1024);
			}


			/*for (pos2 = pos->second->m_ThreadMap.begin(); pos2 != pos->second->m_ThreadMap.end(); ++pos2)
			{
				if(!(pos2->second->m_pFTPClient->m_IsThisPieceComplete))
				{
					break;
				}
			}
			if(pos->second->m_ThreadMap.end() == pos2 && pos->second->m_PieceNum == pos->second->m_ThreadMap.size())
			{
				pos->second->m_bTaskComplete = true;
			}*/
			if (pos->second->m_PieceNum == Complete_Num)
				pos->second->m_bTaskComplete = true;
			else
			{
				//try to implement 'jieli',make it at least there are 2 threads to download
				//if (pos->second->m_PieceNum > 1 && pos->second->m_PieceNum - Complete_Num == 1 && bREST)
				if ((pos->second->m_PieceNum > 1 && pos->second->m_PieceNum < 5 
						&& pos->second->m_PieceNum - Complete_Num == 1 && bREST)
						|| (pos->second->m_PieceNum > 5 && pos->second->m_PieceNum - Complete_Num < 5 && bREST))
				{
					ACE_INT64 itmp = 0;
					//for (int iloop=0;iloop<3;iloop++)
					{
						pCon = NULL;
						for (pos2 = pos->second->m_ThreadMap.begin(); pos2 != pos->second->m_ThreadMap.end(); ++pos2)
						{
							if (pos2->second->m_pFTPClient->m_IsThisPieceComplete)
							{
								pCon = pos2->second;
							}
							else if(NULL != pCon)
							{
								itmp = pos2->second->m_pFTPClient->m_RangeEnd - pos2->second->m_pFTPClient->m_offset;
								if(itmp > 64*1024)//64k
								{
									--Complete_Num;
									//correct the speed datas
									pos->second->m_pre_offset -= (pCon->m_pFTPClient->m_offset-pCon->m_pFTPClient->m_RangeStart)/1024;
									offset -= (pCon->m_pFTPClient->m_offset-pCon->m_pFTPClient->m_RangeStart)/1024;

									//init the jieli thread
									pCon->m_pFTPClient->m_IsThisPieceComplete = false;
									pCon->m_pFTPClient->m_RangeStart =
										pos2->second->m_pFTPClient->m_offset + itmp/2;
									pCon->m_pFTPClient->m_offset = pCon->m_pFTPClient->m_RangeStart;
									pCon->m_pFTPClient->m_RangeEnd = pos2->second->m_pFTPClient->m_RangeEnd;
									pCon->m_pFTPClient->m_fPercent = 0;
									pCon->m_RangeStart = pCon->m_pFTPClient->m_RangeStart;
									pCon->m_uPieceLen = pCon->m_pFTPClient->m_RangeEnd - pCon->m_pFTPClient->m_RangeStart + 1;


									//update the jieli-ed thread
									pos2->second->m_pFTPClient->m_RangeEnd = pCon->m_RangeStart - 1;
									pos2->second->m_uPieceLen = pos2->second->m_pFTPClient->m_RangeEnd - pos2->second->m_RangeStart + 1;

									//active the jieli thread
									if (NULL != pCon)
									{
										while(-1 == pCon->activate(THR_NEW_LWP|THR_JOINABLE \
													|THR_INHERIT_SCHED|THR_SUSPENDED))
										{
											//ACE_OS::sleep(1);
											std::cout<<"activate thread failed!maybe the thread number \
												is too large!"<<std::endl;
										}
										pCon->resume();
										QLOG("<<<<<<<<<<<<<<<<<<<Start a jieli thread successfully>>>>>>>>>>>>>>>>.\n");
										pCon = NULL;
									}
									break;
								}
							}
						}
					}
				}

			}

			FTPGMutex.release();




			if (bFTP_DP_GOON)
			{
				//we need to re-get the global rate number, 
				//but it's hard to get the accurate data,so at the end we turn to decide if the task is completed.
				perc = ((double)iPartFileSize/pos->second->m_FileSize)*100 + gFTP_Rate;
				if(pos->second->m_bTaskComplete)
					perc = 100;
			}
			else
			{
				perc = int((percent/pos->second->m_PieceNum)*100);
			}



			if (false == CFTPDownloadTask::m_bFirstDisplay)
			{
				std::cout<<std::setw(6)<<std::right<<"\r=> %"<<perc<<"|speed:"<<std::setw(10)<<std::right<<(offset-pos->second->m_pre_offset)/t_piece  \
					<<"KB/S |" <<" arg speed: "<<std::setw(10)<<std::right<<std::setprecision(4)<<offset/pos->second->time<<"KB/S |"  \
					<<" time: "<<std::setw(6)<<std::right<<pos->second->time<<" s";
				CFTPDownloadTask::m_bFirstDisplay= true;
				pos->second->m_fSpeed = (offset-pos->second->m_pre_offset)/t_piece;
				pos->second->m_iPerc = perc;
			}
			else
			{
				std::cout<<std::setw(6)<<std::right<<"\r=> %"<<perc<<" | speed: "<<std::setw(10)<<std::right<<(offset-pos->second->m_pre_offset)/t_piece \
					<<"KB/S |" <<" arg speed: "<<std::setw(10)<<std::right<<std::setprecision(4)<<offset/pos->second->time<<"KB/S |" \
					<<" time: "<<std::setw(6)<<std::right<<pos->second->time<<" s";
				pos->second->m_fSpeed = (offset-pos->second->m_pre_offset)/t_piece;
				pos->second->m_iPerc = perc;
			}
			std::cout.flush();
			pos->second->m_pre_offset = offset;



			//next step, to check if task is completed.
			if (pos->second->m_bTaskComplete)
			{
				std::cout<<"  Complete!"<<std::endl;
				QLOG("  Complete!\n\n\n");
				//delete the dp log datas
				std::string strTmpFileName = "." + pos->second->m_strFileName;
				while(0 != remove(strTmpFileName.c_str()))
					;
				delete pos->second;
				pos->second = NULL;
				m_TaskMap.clear();
				//FTPGMutex.release();
				ACE_Reactor::instance()->cancel_timer (this);
				exit(0);
			}

			//update the DP log datas.
			//the log format:
			// [rate]|rangestart1:rangeend1|rangestart2:rangeend2...
			strDP_Log = "[" + ACE_INT64_ITOA(perc, 10) + "]" + strDP_Log;
			strDP_Log = "[" + strServerIp + "]" + strDP_Log;
			CFileStore::CreateFile("."+pos->second->m_strFileName, 0);
			pos->second->m_FileStore.StoreData("."+pos->second->m_strFileName, 0, (char*)strDP_Log.c_str(), strDP_Log.length());
		}	
		else
		{
			QLOG("Impossible.\n");
		}
	}//end of for()

	//FTPGMutex.release();
	return 1;
}
CFTPDownloadTaskManager::~CFTPDownloadTaskManager()
{
	//FTPGMutex.acquire();
	std::map<long, CFTPDownloadTask*>::iterator pos;
	for (pos = m_TaskMap.begin(); pos != m_TaskMap.end();)
	{
		if(NULL != pos->second)
		{
			delete pos->second;
			pos->second = NULL;
		}
		pos++;
		//m_TaskMap.erase(pos);
		//pos = m_TaskMap.begin();
	}
	m_TaskMap.clear();
	//FTPGMutex.release();
}

#endif //_FTP_DECODE_CPP


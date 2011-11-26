#ifndef _HTTP_DECODE_CPP
#define _HTTP_DECODE_CPP

#include "httpdecode.h"
#include "log.h"
#include <cmath>
#include <cstring>
#include <algorithm>
#include "func.h"
#include <ace/Recursive_Thread_Mutex.h>
#include <ace/Mutex.h>

ACE_Recursive_Thread_Mutex GMutex;
//static ACE_Mutex GMutex;
//static ACE_RW_Mutex GMutex;

std::string strVersion = "HTTP/1.1";
//std::string strVersion = "HTTP/1.0";
std::string strChangeLine = "\r\n";
std::string strSpace = " ";
std::string strUserAgent = "User-Agent: QuickGet/0.01 ";//
//std::string strUserAgent = "User-Agent: wget/0.01 ";//
//std::string strAccept = "Accept: text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5";
//std::string strAcceptLang = "Accept-Language: zh-cn,zh;q=0.5";
std::string strAccept = "Accept: */*";
//std::string strAcceptLang = "";
static ACE_UINT32 uServerIp = 0;
static std::string strServerIp = "";
static bool bSupportHeadMethord = true;
static bool bChunked = false;
bool bHTTP_DP_GOON = false;
int gHTTP_Rate = 0;
static int Complete_Num = 0;
static ACE_INT64 overloadpiece_len  = 0;
static float overloadpiece_percent = 0.0;

namespace gHttpResCode
{
	static std::string strOk = "200";//OK
	static std::string strCreated = "201";//Created
	static std::string strAccepted = "202";//Accepted
	static std::string strNoContent = "204";//No Content
	static std::string strPartial = "206";//partial
	static std::string strMovedPerm = "301";//Moved Permanently
	static std::string strMovedTmp = "302";//Moved Temporarily
	static std::string strNotModified = "304";//Not Modified
	static std::string strBadReq = "400";//Bad Request
	static std::string strUnauth= "401";//Unauthorized
	static std::string strForbidden = "403";//Forbidden
	static std::string strNotFound = "404";//Not Found
	static std::string strNotImp = "501";//Not Implemented
	static std::string strBadGW = "502";//Bad Gateway
	static std::string strServiceUnavailable = "503";//Service Unavailable
};

namespace gReqMethord
{
	static std::string strGet = "GET";
	static std::string strHead = "HEAD";
	static std::string strPost = "POST";

};

static ACE_INT64 HEX_ATOI(const char *input,int len)
{
	ACE_INT64 value = 0;
	int i =0;
	int tmplen = len;
	while (tmplen > 0)
	{

		switch (input[tmplen-1])
		{
			case 'F':
			case 'f':
				value += 15*pow(16,i);
				break;
			case 'E':
			case 'e':
				value += 14*pow(16,i);
				break;
			case 'D':
			case 'd':
				value += 13*pow(16,i);
				break;
			case 'C':
			case 'c':
				value += 12*pow(16,i);
				break;
			case 'B':
			case 'b':
				value += 11*pow(16,i);
				break;
			case 'A':
			case 'a':
				value += 10*pow(16,i);
				break;
			case '9':
				value += 9*pow(16,i);
				break;
			case '8':
				value += 8*pow(16,i);
				break;
			case '7':
				value += 7*pow(16,i);
				break;
			case '6':
				value += 6*pow(16,i);
				break;
			case '5':
				value += 5*pow(16,i);
				break;
			case '4':
				value += 4*pow(16,i);
				break;
			case '3':
				value += 3*pow(16,i);
				break;
			case '2':
				value += 2*pow(16,i);
				break;
			case '1':
				value += 1*pow(16,i);
				break;
			case '0':
				break;
			default:
				std::cout<<"Wrong input"<<std::endl;
				QLOG("Wrong input to func HEX_ATOI\n");
				break;
		}
		++i;
		--tmplen;
	}
	return value;
}
/*
   ACE_INT64 ACE_INT64_ATOI(const char * input, int len)
   {
   ACE_INT64 ret_value = 0;
   int i  =0;
   while (len>0)
   {
   ret_value += (input[len-1]-'0') * pow(10,i);
   ++i; --len;
   }
   return ret_value;
   }

   std::string ACE_INT64_ITOA(ACE_INT64 iNum, char *buf, int index)
   {
//std::cout<<"before process :"<<iNum<<std::endl;
std::string strRet = "";
char aa = '0';
if (iNum == 0)
strRet += "0";
while(iNum>0)
{
aa = (char)(iNum%index + '0');
strRet.insert(strRet.begin(),aa);
iNum = iNum / index;
}	
//strRet.reverse(strRet.begin(), strRet.end());
//std::cout<<"after process :"<<strRet<<std::endl;
return strRet;
}
*/
//begin CHttpClient---------------------------------------------
CHttpClient::CHttpClient(int iPieceID): m_PieceID(iPieceID)
{
	m_strFileName = "";
	m_strDomain = "";
	m_offset = 0;
	m_IsThisPieceComplete = false;
	m_iPieceNum = 0;
	//m_PieceID = iPieceID;
	fflag = false;
	m_fPrecent = 0.000000;
	m_iFileSize = 0;
	m_bChunk = false;
	m_bRevClose = false;
	m_ChunkLeftLen = 0;
	m_RangeStart = 0;
	m_RangeEnd = 0;
	m_jieli_updaterange = false;
}

bool CHttpClient::bEncodeRequest(std::string pstrRequestURL, std::string& strEncodedRequest, std::string strReqMethord, ACE_INT64 RangeStart, ACE_INT64 RangeEnd)
{
	//std::cout<<"Enter HttpClient::bEncodeRequest!"<<std::endl;
	//std::cout<<"Range: "<<RangeStart<<":"<<RangeEnd<<std::endl;
	//GET / HTTP/1.1\r\n
	//Host: www.google.com
	//User-Agent: QuickGet/0.01 (suselinux) \r\n
	//Accept: text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5\r\n
	//Accept-Language: zh-cn,zh;q=0.5\r\n
	//Accept-Charset: gb2312,utf-8;q=0.7\r\n
	//Range: 
	//Keep-Alive: 300\r\n
	//Connection: keep-alive\r\n
	//\r\n                                            .

	//Big file need to update the range value
	m_RangeStart = RangeStart;
	m_RangeEnd = RangeEnd;

	std::string strURL = pstrRequestURL;
	std::string strFlag = "://";
	std::string::size_type iPos1 = strURL.find(strFlag, 0); 
	if (iPos1 == std::string::npos)
	{
		return false;
	}
	//should do more work about the file name, need to check the URL to define the filename.
	std::string::size_type iPos2 = strURL.find('/',iPos1 + 3);
	if (iPos2 == std::string::npos)
	{
		m_strDomain = strURL.substr(iPos1+3, strURL.length()-(iPos1+3)+1);
		//std::cout<<"do main: "<<m_strDomain<<std::endl;
		QLOG("do main: %s\n", m_strDomain.c_str());
		strEncodedRequest += strReqMethord + strSpace + "/" + strSpace;
		//strEncodedRequest += strSpace;
		strEncodedRequest += strVersion + strChangeLine;
	}
	else
	{ 
		m_strDomain = strURL.substr(iPos1+3,iPos2 - (iPos1 +3));
		strEncodedRequest += strReqMethord + strSpace;
		strEncodedRequest += strURL.substr(iPos2, strURL.length() - (iPos2) + 1);
		strEncodedRequest += strSpace;
		strEncodedRequest += strVersion + strChangeLine;
		std::string::size_type iPos3 = strURL.rfind('/');
		//m_strFileName = strURL.substr(iPos3+1);
	}

	char tmp[50];
	ACE_OS::memset(tmp, 0, sizeof(tmp));
	//if (RangeStart != RangeEnd && m_iPieceNum > 1)
	if (RangeStart != RangeEnd && bSupportHeadMethord)
	{
		std::string strRange = std::string("Range: bytes=") + std::string(ACE_INT64_ITOA(RangeStart, tmp, 10)) + std::string("-") + std::string(ACE_INT64_ITOA(RangeEnd, tmp, 10));
		strEncodedRequest += strRange + strChangeLine;
	}

	/*
	   bool bFileNameFlag = false;
	   if (m_strFileName == std::string("") || m_strFileName == std::string("/"))
	   {
	   m_strFileName = m_strDomain + ".html";
	   bFileNameFlag = true;
	   }

	   if (m_iPieceNum > 1)
	   {
	//m_strFileName += ACE_OS::itoa(m_PieceID, tmp, 10);
	goto AA1;
	}
	//std::cout<<"File Name: "<<m_strFileName<<std::endl;
	QLOG("File Name: %s\n", m_strFileName.c_str());
	*/

AA1:

	//domain
	strEncodedRequest += "Host: " + m_strDomain + strChangeLine;
	//UserAgent
	strEncodedRequest += strUserAgent + strChangeLine;
	//AcceptType
	strEncodedRequest += strAccept + strChangeLine;
	//Language
	//strEncodedRequest += strAcceptLang + strChangeLine;


	//strEncodedRequest += "Keep-Alive: 300" + strChangeLine;
	strEncodedRequest += "Connection: keep-alive" + strChangeLine;

	//next stage function
	//Accept-Encoding: gzip,deflate,no need to implement this, this is not a browser.

	strEncodedRequest += strChangeLine;

	//std::cout<<"2Leave HttpClient::bEncodeRequest!"<<std::endl;
	//std::cout<<"Req:-------------------------------->\n"<<strEncodedRequest<<std::endl;
	//std::cout<<"encode request ok!"<<std::endl;
	QLOG("Req:--------------------------------->\n");
	QLOG("encode request ok!\n");
	QLOG("encode request : %s\n", strEncodedRequest.c_str());
	return true;
}

bool CHttpClient::bConnectClose(std::string res_header)
{
	std::string strFlag = "Connection:";
	std::transform (strFlag.begin(), strFlag.end(), strFlag.begin(), toupper);
	std::string strHeader = res_header;
	std::transform (strHeader.begin(), strHeader.end(), strHeader.begin(), toupper);
	std::string::size_type iPos1 = strHeader.find(strFlag, 0);
	if (std::string::npos == iPos1)
	{
		QLOG("No Header: Connection:\n");
		return false;
	}
	std::string::size_type iPos2 = strHeader.find("\r\n", iPos1);
	std::string str_con_status = strHeader.substr(iPos1 + ACE_OS::strlen("Connection:"), iPos2-(iPos1 + ACE_OS::strlen("Connection:")));
	Trim(str_con_status);
	std::transform (str_con_status.begin(), str_con_status.end(), str_con_status.begin(), toupper);
	std::string str_aim = "CLOSE";
	if (str_con_status == str_aim)
		return true;

	return false;
}

// Transfer-Encoding: chunked
bool CHttpClient::bChunked(std::string res_header)
{
	std::string strFlag = "Transfer-Encoding:";
	std::transform (strFlag.begin(), strFlag.end(), strFlag.begin(), toupper);
	std::string strHeader = res_header;
	std::transform (strHeader.begin(), strHeader.end(), strHeader.begin(), toupper);
	std::string::size_type iPos1 = strHeader.find(strFlag, 0);
	if (std::string::npos == iPos1)
	{
		QLOG("No Header: Transfer-Encoding:\n");
		return false;
	}
	std::string::size_type iPos2 = strHeader.find("\r\n", iPos1);
	std::string str_encoding = strHeader.substr(iPos1 + ACE_OS::strlen("Transfer-Encoding:"), iPos2-(iPos1 + ACE_OS::strlen("Transfer-Encoding:")));
	Trim(str_encoding);
	std::transform (str_encoding.begin(), str_encoding.end(), str_encoding.begin(), toupper);
	std::string str_aim = "CHUNKED";
	if (str_encoding == str_aim)
		return true;
	return false;
}




/*
   A process for decoding the "chunked" transfer coding (section 3.6)
   can be represented in pseudo-code as:

length := 0
read chunk-size, chunk-ext (if any) and CRLF
while (chunk-size > 0) {
read chunk-data and CRLF
append chunk-data to entity-body
length := length + chunk-size
read chunk-size and CRLF
}
read entity-header
while (entity-header not empty) {
append entity-header to existing header fields
read entity-header
}
Content-Length := length
Remove "chunked" from Transfer-Encoding
*/


unsigned int CHttpClient::DecodeResponse(const char*resdata, int len)
{
	char *res = (char*)resdata;
	char *data = NULL;
	int headersize = 0;
	bool endflag1 = false;
	bool endflag2 = false;
	int i =0;
	int j =0;
	char *pHeader = NULL;
	ACE_INT64 iContentLen = 0;
	std::string tmp= "";
	std::string strResCode="";


	if (m_ChunkLeftLen != 0)
	{
		//this is for receiving 'one not finished chunked data'
		GMutex.acquire();
		QLOG("chuncked data receiving...\n");
		int tmpdatalen = len - headersize;

		unsigned long ChunkLen = 0;
		std::string strData = "";
		std::string strChunkData = "";

		std::string::size_type idx_chunk1 = 0;
		std::string::size_type idx_chunk2 = 0;
		std::string strChunkLen="";
		strData = std::string(res + headersize);
		if (m_ChunkLeftLen != 0)
		{
			if (strData.length() >= m_ChunkLeftLen)
			{
				m_FileStore.StoreData(m_strFileName, m_offset, (char*)strData.c_str(), m_ChunkLeftLen);
				m_offset += m_ChunkLeftLen;
				strData = std::string(res + headersize + m_ChunkLeftLen);
				m_ChunkLeftLen = 0;
				m_IsThisPieceComplete = true;
				Complete_Num++;
			}
			else
			{
				m_FileStore.StoreData(m_strFileName, m_offset, (char*)strData.c_str(), strData.length());
				m_offset += strData.length();
				m_ChunkLeftLen -= strData.length();
				if (m_ChunkLeftLen == 0)
				{
					m_IsThisPieceComplete = true;
					Complete_Num++;
				}
			}
		}
		GMutex.release();
		return 0;
	}

	//jieli-ed, so stop and update the rangeStart and rangeEnd.
	GMutex.acquire();
	if(m_jieli_updaterange)
	{		
		m_jieli_updaterange = false;
		GMutex.release();
		return -1;
	}
	GMutex.release();

	/*
	 * Currently,    all the response cases:
	 * 1. res code and headers,  ended with a "\r\n\r\n" ;
	 * 2. pure data, no rescode code,  no headers, just data ;
	 * 3. 
	 */
	//what's this?this is for filter the header from the data,well stupid!
	//get the header end position.(find the first "\r\n\r\n", and it's the end of the header)
	for (i=0;i<len ; ++i)
	{
		//if ((res[i] == '\r') && (res[i+1] == '\n') && (res[i+2] == '\r') && (res[i+3] == '\n'))
		//enhanced by hgj for more testing
		if (i<len && res[i] == '\r')
		{
			while(i<len && (res[i+1] == ' ' || res[i+1] == '	' ) )
				++i;
			++i;
			if(i<len && res[i] == '\n')
			{
				while(i<len && (res[i+1] == ' ' || res[i+1] == '	' ) )
					++i;
				++i;
				if(i<len && res[i] == '\r')
				{
					while(i<len && (res[i+1] == ' ' || res[i+1] == '	' ) )
						++i;
					++i;
					if(i<len && res[i] == '\n')
					{
						endflag1 = true;
						QLOG("get 1  \\r\\n\\r\\n!\n");
						break;
					}
				}
			}
		}
	}
	if(!endflag1)
	{
		;//QLOG("can not find \\r\\n\\r\\n in the res header,maybe the header is not completed,need goon receive.\n");
	}

	//this a second check for the existing of the header.(find the string "HTTP/1.1")
	//for (; j<10; ++j)
	/*for (; j+7<len; ++j)
	  {
	  if ( (res[j] == 'H' || res[j] == 'h') && (res[j+1] == 'T' || res[j+1] == 't') 
	  && (res[j+2] == 'T' || res[j+2] == 't') && (res[j+3] == 'P' || res[j+3] == 'p') 
	  && (res[j+4] == '/') && (res[j+5] == '1') && (res[j+6] == '.') && ((res[j+7] == '1')||(res[j+7] == '0')) )
	  {
	  endflag2 = true;
	  break;
	  }			
	  }*/	
	for (j=0; j<len; ++j)
	{
		if (j<len && (res[j] == 'H' || res[j] == 'h'))//h
		{
			while (j<len && (res[j+1] ==' ' || res[j+1] == '	'))
				++j;
			++j;
			if(j<len && (res[j] == 'T' || res[j] == 't'))//t
			{
				while (j<len && (res[j+1] == ' ' || res[j+1] == '	'))
					++j;
				++j;
				if (j<len && (res[j] == 'T' || res[j] == 't'))//t
				{
					while (j<len && (res[j+1] ==' ' || res[j+1] == '	'))
						++j;
					++j;
					if (j<len && (res[j] == 'P' || res[j] == 'p'))//p
					{
						while (j<len && (res[j+1] ==' ' || res[j+1] == '	'))
							++j;
						++j;
						if (j<len && (res[j] == '/'))// /
						{
							while (j<len && (res[j+1] ==' ' || res[j+1] == '	'))
								++j;
							++j;
							if (j<len && (res[j] == '1'))//1
							{
								while (j<len && (res[j+1] ==' ' || res[j+1] == '	'))
									++j;
								++j;
								if (j<len && (res[j] == '.'))// .
								{
									while (j<len && (res[j+1] ==' ' || res[j+1] == '	'))
										++j;
									++j;
									if (j<len && (res[j] == '1' || res[j] == '0'))
										// 1 or 0. http/1.1 or http/1.0
									{
										endflag2 = true;
										break;
									}
								}
							}
						}
					}
				}
			}

		}			
	}	

	if(!endflag2)
	{
		;//QLOG("can not find HTTP/1.1 in the res header,maybe the header is not completed,need goon receive.\n");
	}
	if(!endflag1 && !endflag2)
	{
		;//QLOG("get pure data response.\n");
	}

	if(endflag1 != endflag2)
	{
		QLOG("CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC-----the header maybe not completed,need goon receive.\n");
		QLOG("CCCCCCCCCCC-----endflag1: %d, endflag2: %d.\n", endflag1, endflag2);
		QLOG("CCCCCCCCCCC-----Res: %s\n", res);

	}


	//re-write the match func
	/*std::string strResMatch = res;
	  std::string strHeaderBeginFlag = "HTTP/1.1";
	  std::string strHeaderEndFlag = "\r\n\r\n";
	  transform (strResMatch.begin(), strResMatch.end(), strResMatch.begin(), toupper);
	  Trim(strResMatch)
	  */

	if (endflag1 && endflag2)
	{
		QLOG("get 1  real header !\n");
		//this is not stable?
		//data = res+i+4;//data start position.
		//headersize = i+4;//headersize.
		data = res+i+1;//data start position.
		headersize = i+1;//headersize.

		//try to find another header
		//i = i+3;
		i = i;
		if(i<len)
		{
			j = i;
			for (; j+7<len; ++j)
			{
				if ( (res[j] == 'H' || res[j] == 'h') && (res[j+1] == 'T' || res[j+1] == 't')
						&& (res[j+2] == 'T' || res[j+2] == 't') && (res[j+3] == 'P' || res[j+3] == 'p')
						&& (res[j+4] == '/') && (res[j+5] == '1') && (res[j+6] == '.') && (res[j+7] == '1'))
				{
					QLOG("DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD,find another res header begin.\n");
					break;
				}
			}

			for (; i+3<len; ++i)
			{
				if ((res[i] == '\r') && (res[i+1] == '\n') && (res[i+2] == '\r') && (res[i+3] == '\n'))
				{
					QLOG("DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD,find another res header end.\n");
					break;
				}
			}
		}
	}
	else
	{
		data = res;//no header
		headersize = 0;
	}
	pHeader = new char[headersize+1];
	memset(pHeader, 0, headersize+1);
	for (int ii =0;ii<headersize;++ii)
		*(pHeader+ii) = *(res+ii);

	//tmp = pHeader;
	if (0 != headersize)
	{
		tmp = pHeader;
		QLOG("Res Header: %s\n", pHeader);

		GetResCode(tmp, strResCode);
		if (strResCode == gHttpResCode::strOk || strResCode == gHttpResCode::strPartial)
		{
			if (!m_bSupportHeadMethord)
			{
				if (m_iFileSize == 0)
				{
					iContentLen = GetContentLen(tmp);
					if (0 != iContentLen)
					{
						QLOG("Get filesize from res msg, FileSize: %Q, Bytes\n", iContentLen);
						m_iFileSize = iContentLen;
					}
					else//iContentLen = 0
					{
						if(!m_bChunk && bChunked(tmp))
						{
							m_bChunk = true;
						}
						if (m_bChunk)//chunked processing
						{
							QLOG("chuncked encoding,begin receive----------->\n");
							int tmpdatalen = len - headersize;

							unsigned long ChunkLen = 0;
							std::string strData = "";
							std::string strChunkData = "";

							std::string::size_type idx_chunk1 = 0;
							std::string::size_type idx_chunk2 = 0;
							std::string strChunkLen="";
							strData = std::string(res + headersize);
							do 
							{
								/*if (m_ChunkLeftLen != 0)//still for 'one not finished chunk receving'
								  {
								  QLOG("m_ChunkLeftLen != 0!----------->\n");
								  if (strData.length() >= m_ChunkLeftLen)
								  {
								  m_FileStore.StoreData(m_strFileName, m_offset, (char*)strData.c_str(), m_ChunkLeftLen);
								  m_offset += m_ChunkLeftLen;
								  strData = std::string(res + headersize + m_ChunkLeftLen);
								  m_ChunkLeftLen = 0;
								  }
								  else
								  {
								  m_FileStore.StoreData(m_strFileName, m_offset, (char*)strData.c_str(), strData.length());
								  m_offset += strData.length();
								  m_ChunkLeftLen -= strData.length();
								  break;
								  }
								  }*/

								QLOG("a chunk Beginning, ----------------m_ChunkLeftLen = 0\n ");
								//try to get the chunk size,find the first \r\n
								idx_chunk2 = strData.find("\r\n");//find  the datalen boundary
								if (idx_chunk2 != std::string::npos)
								{
									QLOG("Find chunk len string\n");
									strChunkLen = strData.substr(0, idx_chunk2-idx_chunk1);
									Trim(strChunkLen);
									QLOG("chunk len string : %s\n", strChunkLen.c_str());
									ChunkLen = HEX_ATOI(strChunkLen.c_str(), strChunkLen.length());
									QLOG("ChunkDataLen: %d\n", ChunkLen);
									if (ChunkLen == 0)
									{
										QLOG("one chunked data receive finished.\n");
										m_IsThisPieceComplete = true;
										Complete_Num++;
										goto CHUNK_OK;
									}
									//update the data pointer
									strData = strData.substr(idx_chunk2+2);

									/*if (tmpdatalen-strChunkLen.length()-2-2 >= ChunkLen)
									  {
									  strChunkData = strData.substr(strChunkLen.length()+2, ChunkLen);
									  m_FileStore.StoreData(m_strFileName, m_offset, (char*)strChunkData.c_str(), ChunkLen);
									  m_offset += ChunkLen;
									  strData = std::string(res+headersize+ strChunkLen.length()+2+ChunkLen+2);//jump to next data
									  idx_chunk1 = idx_chunk2=0;
									  }
									  else*/
									{	//write the received data to file directly，and mark how much need to receive
										int received_datalen = strData.length()-2;
										m_FileStore.StoreData(m_strFileName, m_offset, (char*)strData.c_str(), received_datalen);
										m_offset += received_datalen;
										m_ChunkLeftLen = ChunkLen - received_datalen;
										QLOG("one chunk: size: %d | left: %d\n", ChunkLen, m_ChunkLeftLen);
										break;
									}
								}//if (idx_chunk2 != std::string::npos)
								else
								{
									QLOG("still Not Find chunk len string yet.\n ");
									//there is no data,just header,so no need to write to file
									//m_FileStore.StoreData(m_strFileName, m_offset, (char*)strData.c_str(), strData.length());
									//m_offset += strData.length();
									break;
								}
								//}while(ChunkLen != 0);
							}while(0);//we should make it to go on receiving data from socket
CHUNK_OK:
							QLOG("Chuncked data received OK!----------->\n");
							/*if(bConnectClose(tmp))
							  {
							  QLOG("find Connection: Close, so close the connection.\n");
							  m_bRevClose = true;
							//m_IsThisPieceComplete = true;
							}*/
							delete []pHeader;
							pHeader = NULL;
							//GMutex.release();
							return 0;
						}//end of if (m_bChunk)
					}//end of iContentLen = 0	
				}//end of if (m_iFileSize == 0)
			}//end of  if (!m_bSupportHeadMethord)
		}//if (strResCode == gHttpResCode::strOk || strResCode == gHttpResCode::strPartial)
		else
		{
			//std::cout<<"ResCode: "<<strResCode<<"－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－＞"<<std::endl;
			//std::cout<<"header:"<<pHeader<<std::endl;
			QLOG("**********Error ResCode: %s\n", strResCode.c_str());
			QLOG("the response Header: \n%s\n", pHeader);
			delete []pHeader;
			pHeader = NULL;
			//GMutex.release();
			return -1;
		}
	}//end of if (0 != headersize)
	else//headersize = 0
	{
		// go on receiving chunked data; a series chunked data, the last one 's length is 0
		if(m_bChunk && !m_IsThisPieceComplete)
		{
		QLOG("go on receiving chuncked data----------->\n");
		int tmpdatalen = len - headersize;

		unsigned long ChunkLen = 0;
		std::string strData = "";
		std::string strChunkData = "";

		std::string::size_type idx_chunk1 = 0;
		std::string::size_type idx_chunk2 = 0;
		std::string strChunkLen="";
		strData = std::string(res + headersize);


		QLOG("a chunk Beginning, ----------------m_ChunkLeftLen = 0\n ");
		//try to get the chunk size,find the first \r\n
		idx_chunk2 = strData.find("\r\n");
		if (idx_chunk2 != std::string::npos)
		{
			QLOG("Find chunk len string\n");
			strChunkLen = strData.substr(0, idx_chunk2-idx_chunk1);
			Trim(strChunkLen);
			QLOG("chunk len string : %s\n", strChunkLen.c_str());
			ChunkLen = HEX_ATOI(strChunkLen.c_str(), strChunkLen.length());
			QLOG("ChunkDataLen: %d\n", ChunkLen);
			if (ChunkLen == 0)
			{
				QLOG("one chunked data receive finished.\n");
				//if server did not close the connection, there are maybe other data.
				GMutex.acquire();
				m_IsThisPieceComplete = true;
				Complete_Num++;
				GMutex.release();
				goto CHUNK_OK2;
			}
			//update the data pointer
			strData = strData.substr(idx_chunk2+2);
			int received_datalen = strData.length()-2;
			m_FileStore.StoreData(m_strFileName, m_offset, (char*)strData.c_str(), received_datalen);
			GMutex.acquire();
			m_offset += received_datalen;
			m_ChunkLeftLen = ChunkLen - received_datalen;
			GMutex.release();
			QLOG("one chunk: size: %d | left: %d\n", ChunkLen, m_ChunkLeftLen);
		}
		else
		{
			QLOG("still Not Find chunk len string yet.\n ");
		}
		/*if(bConnectClose(tmp))
		  {
		  QLOG("find Connection: Close, so close the connection.\n");
		  m_bRevClose = true;
		  m_IsThisPieceComplete = true;
		  }*/

CHUNK_OK2:
		delete []pHeader;
		pHeader = NULL;
		return 0;
	}//if(m_bChunk && !m_IsThisPieceComplete)
	}
	if(NULL != pHeader)
	{
		delete []pHeader;
		pHeader = NULL;
	}

	int datasize = len - headersize;
	//is it need to check the overflow for http's data receiving?  needed.
	// we can not reply on the server,so we need to do control.
	GMutex.acquire();
if (m_offset + datasize > m_RangeEnd+1 && m_bSupportHeadMethord)
{
	QLOG("over flow detected --------m_offset: %Q  m_RangeEnd: %Q \n", m_offset+datasize, m_RangeEnd);
	m_FileStore.StoreData(m_strFileName, m_offset, data, (m_RangeEnd+1)-m_offset);
	m_offset += (m_RangeEnd+1)-m_offset;
	QLOG("piece %d complete! \n", m_PieceID);
	m_IsThisPieceComplete = true;
	Complete_Num++;
	m_fPrecent = ((float)(m_offset- m_RangeStart +1))/(m_RangeEnd - m_RangeStart+1);
	goto rev_ok;
}
else
{
	m_FileStore.StoreData(m_strFileName, m_offset, data, datasize);
	m_offset += datasize;
}


chunk_ok:
if (!m_bSupportHeadMethord)
{
	if(m_bChunk)
	{
		if (m_bRevClose)
		{
			QLOG("piece %d complete! \n", m_PieceID);
			m_IsThisPieceComplete = true;
			Complete_Num++;
		}
	}
	else
	{
		m_fPrecent = ((float)m_offset)/(m_iFileSize);
		if (m_offset == m_iFileSize)
		{
			QLOG("piece %d complete! \n", m_PieceID);
			m_IsThisPieceComplete = true;
			Complete_Num++;
		}
	}
}
else
{
	m_fPrecent = ((float)(m_offset- m_RangeStart +1))/(m_RangeEnd - m_RangeStart+1);
	if (m_offset == m_RangeEnd + 1)
	{
		QLOG("piece %d complete! \n", m_PieceID);
		m_IsThisPieceComplete = true;
		Complete_Num++;
	}
}

rev_ok:
GMutex.release();
return datasize;
}

bool CHttpClient::GetResCode(std::string strTmpRes, std::string &ResCode)
{
	QLOG("Enter HttpClient::GetResCode!\n");
	std::string strRes = strTmpRes;
	std::string::size_type iPos1 = strRes.find("\r\n", 0);
	std::string strLineOne = strRes.substr(0, iPos1-0);
	QLOG("LineOne: %s\n", strLineOne.c_str());
	std::string::size_type i = strLineOne.find("HTTP/1.1", 0);
	if(i == std::string::npos)
	{
		i = strLineOne.find("HTTP/1.0", 0);
	}
	strLineOne = strLineOne.substr(i+8);
	Trim(strLineOne);
	ResCode = strLineOne.substr(0,3);
	return true;
}

ACE_INT64 CHttpClient::GetContentLen(std::string strTmpRes)
{
	std::string strFlag = "Content-Length:";
	std::transform (strFlag.begin(), strFlag.end(), strFlag.begin(), toupper);
	std::string strRes = strTmpRes;
	std::transform (strRes.begin(), strRes.end(), strRes.begin(), toupper);
	std::string::size_type iPos1 = strRes.find(strFlag, 0);
	if (std::string::npos == iPos1)
	{
		QLOG("No Header: Content-Length\n");
		return 0;
	}
	std::string::size_type iPos2 = strRes.find("\r\n", iPos1);
	std::string strNum = strRes.substr(iPos1 + ACE_OS::strlen("Content-Length:"), iPos2-(iPos1 + ACE_OS::strlen("Content-Length:")));
	Trim(strNum);
	ACE_INT64 iLen = ACE_INT64_ATOI(strNum.c_str(), strNum.length());

	return iLen;
}

void CHttpClient::Trim(std::string &strInput)
{
	int iLen = strInput.length();
	std::string strNew = "";	

	for (int i = 0;i<iLen;++i)
	{
		if ((' ' == strInput[i]) || ('	' == strInput[i]))
		{
			continue;
		}		
		else
		{
			strNew += strInput[i];
		}
	}

	strInput = strNew;
	return;
}
bool CHttpClient::GetTaskStatus()
{
	/*bool bTmp = false;
	  m_mutex.acquire();
	  bTmp =  CHttpClient::IsTaskComplete;
	  m_mutex.release();
	  return bTmp;*/
	return true;
}

ACE_INT64 CHttpClient::HeadMethodForInfo(std::string &strReqURL, std::string &strFileName)
{
	QLOG("HEAD begin connect......!\n");
	std::string strEncodedRequest = "";
	std::string strDecodedRes = "";
	std::string strReqMethord = "GET";//change to use "GET,not ""HEAD";
	std::string sreReqMothordGet = "GET";

ReDirect: 	
	strEncodedRequest = "";
	strDecodedRes = "";
	//strReqMethord = "HEAD";

	std::string strURL = strReqURL;


	std::string strFlag = "://";
	std::string::size_type iPos1 = strURL.find(strFlag, 0); 
	if (iPos1 == std::string::npos)
	{
		QLOG("did not find :// in url, please check.\n");
		return 0;
	}
	//get domain
	std::string strDomain = "";
	int iPos2 = strURL.find('/',iPos1 + 3);
	if (iPos2 == std::string::npos)
	{
		strDomain = strURL.substr(iPos1+3);
	}
	else
		strDomain = strURL.substr(iPos1+3, iPos2 - (iPos1 +3));

	strEncodedRequest += strReqMethord + strSpace;
	if(iPos2 == std::string::npos)
	{
		strEncodedRequest += "/";
	}
	else
	{
		strEncodedRequest += strURL.substr(iPos2, strURL.length() - (iPos2) + 1);
	}
	strEncodedRequest += strSpace;
	strEncodedRequest += strVersion + strChangeLine;
	strEncodedRequest += "Host: " + strDomain + strChangeLine;
	strEncodedRequest += strUserAgent + strChangeLine;
	strEncodedRequest += strAccept + strChangeLine;
	//strEncodedRequest += strAcceptLang + strChangeLine;
	//strEncodedRequest += "Keep-Alive: 300" + strChangeLine;
	strEncodedRequest += "Connection: keep-alive" + strChangeLine;
	strEncodedRequest += strChangeLine;
	QLOG("HEAD Request: %s\n", strEncodedRequest.c_str());

	ACE_SOCK_Connector Connector;
	ACE_SOCK_Stream SockStream;
	ACE_Time_Value TimeOut(20,0);
	char buf[1024] = {0};
	char str_port[] = "80";
	ACE_INET_Addr addr(str_port, strDomain.c_str());

	// methord 1
	/*
	   uServerIp = addr.get_ip_address();
	   uServerIp = htonl(uServerIp);
	   struct in_addr addr_tmp;
	   memcpy(&addr_tmp, &uServerIp, 4);
	   strServerIp = inet_ntoa(addr_tmp);
	   */

	// methord 2
	if(!bHTTP_DP_GOON)
		strServerIp = addr.get_host_addr();

	// methord 3
	// try to get all ip of one domain.

	QLOG("Serverv ip %s \n", strServerIp.c_str());
	addr = ACE_INET_Addr("80", strServerIp.c_str());

	int iCount  = 0;
	int offset = 0;
ReConnect:	
	if (-1 == Connector.connect(SockStream, addr, &TimeOut))
	{
		QLOG("HAED:    Timeout while connecting server\n");
		iCount+=5;
		if(iCount > 30)
		{
			ACE_OS::sleep(iCount);
			iCount = 0;
		}
		addr = ACE_INET_Addr("80", strServerIp.c_str());
		goto ReConnect;
	}
	QLOG("*************HEAD connect successfully------------------------->\n");
	SockStream.send_n(strEncodedRequest.c_str(), strEncodedRequest.length(), &TimeOut);
	int itmp = SockStream.recv(buf, sizeof(buf), &TimeOut);
	if (0 >= itmp)
	{
		QLOG("send HEAD request failed!will re-connect.\n");
		iCount += 5;
		if(iCount > 30)
		{
			ACE_OS::sleep(iCount);
			iCount = 0;
		}
		itmp = -1;
		ACE_OS::memset(buf, 0, sizeof(buf));
		SockStream.close();
		goto ReConnect;
	}

	std::string strRes = buf;
	QLOG("HEAD Res: %s\n", strRes.c_str());
	std::string strResCode = "";
	CHttpClient::GetResCode(strRes, strResCode);
	if (strResCode == gHttpResCode::strMovedPerm || strResCode == gHttpResCode::strMovedTmp)
	{
		std::string strFlag = "Location:";
		transform (strFlag.begin(), strFlag.end(), strFlag.begin(), toupper);
		std::string strTmpRes = strRes;
		transform (strTmpRes.begin(), strTmpRes.end(), strTmpRes.begin(), toupper);
		std::string::size_type iPos1 = strTmpRes.find(strFlag, 0);
		if (std::string::npos == iPos1)
		{
			QLOG("No Header : Location, so,we can not get a available URL.\n");
			return 0;//it means that there is no this header-item
		}
		std::string::size_type iPos2 = strRes.find("\r\n", iPos1);
		iPos1 += strFlag.length();
		strReqURL = strRes.substr(iPos1, iPos2 - iPos1);
		std::cout<<"Redirect to--------->"<<strReqURL<<std::endl;
		QLOG("Redirect to--------->%s\n", strReqURL.c_str());
		//use the new URI to send request
		goto ReDirect;
	}
	ACE_INT64 iLen = 0;
	if (strResCode == gHttpResCode::strOk)
	{
		iLen = CHttpClient::GetContentLen(strRes);
		QLOG("IN HEAD: %d \n", iLen);
	}
	else
	{
		QLOG("Not support HEAD Methord\nResCode: %s\nWill Get the file directly with one connection!\n", strResCode.c_str());
		SockStream.close();
		//try GET methord,and get the content-len,need to test and verify!
		strReqMethord = "GET";
		goto ReDirect;
	}

	//adjust the filename,this is needed.
	strURL = strURL.substr(7);//http://
	std::string::size_type idx = strURL.rfind('/');
	if (idx == strURL.length()-1)
	{
		strURL = strURL.substr(0, strURL.length()-1);
		strFileName =  strURL + ".html";
		iLen = 0;//make it chunked
	}
	else if (idx == std::string::npos)
	{
		strFileName = strURL + ".html";
		iLen = 0;//make it chunked
	}
	else
	{
		strFileName = strURL.substr(idx+1);
	}
	std::cout<<"FileName: "<<strFileName<<std::endl;
	QLOG("FileName: %s\n", strFileName.c_str());
	return iLen;
}


bool CHttpClient::UpdateRange(std::string &strEncodeReq, ACE_INT64 RangeStart, ACE_INT64 RangeEnd)
{
	std::string strTmp = strEncodeReq;
	int pos1 = strTmp.find("Range: ", 0);
	if (pos1 == std::string::npos)
		return false;
	std::string strTmp1 = strTmp.substr(0, pos1);

	std::string strTmp2 = strTmp.substr(pos1, std::string::npos);
	int pos2 = strTmp2.find_first_of("\r\n", 0);
	std::string strTmp3 = strTmp2.substr(pos2+2, std::string::npos);
	char tmp[50];
	ACE_OS::memset(tmp, 0, sizeof(tmp));
	std::string str1 = ACE_INT64_ITOA(RangeStart, tmp, 10);
	ACE_OS::memset(tmp, 0, sizeof(tmp));
	std::string str2 = ACE_INT64_ITOA(RangeEnd, tmp, 10);
	std::string strRange = std::string("Range: bytes=") + str1 + std::string("-") + str2;
	strEncodeReq = strTmp1 + strRange + strChangeLine + strTmp3;
	QLOG("encode request(updated) : %s\n", strEncodeReq.c_str());

	return true;
}

CHttpConnector::CHttpConnector(std::string strURL, ACE_Time_Value timeout, ACE_INT64 iRangeStart, ACE_INT64 iRangeEnd, int iPieceID, bool bSupportHeadMethord, int iPieceNum ,std::string filename)
{
	m_strURL = strURL;
	m_TimeOut = timeout;
	m_RangeStart = iRangeStart;
	m_RangeEnd = iRangeEnd;
	m_PieceID = iPieceID;
	m_pHttpClient = new CHttpClient(iPieceID);
	m_pHttpClient->m_bSupportHeadMethord = bSupportHeadMethord;
	m_iPieceNum = iPieceNum;
	m_pHttpClient->m_iPieceNum = iPieceNum;
	m_pHttpClient->m_offset = m_RangeStart;
	m_pHttpClient->m_strFileName = filename;
	m_pHttpClient->m_RangeStart = m_RangeStart;
	m_pHttpClient->m_RangeEnd = m_RangeEnd;
	QLOG("HttpConnector created!Range: %Q-%Q\n", m_RangeStart, m_RangeEnd);
}

CHttpConnector::~CHttpConnector()
{
	if (NULL != m_pHttpClient)
	{
		delete m_pHttpClient;
		m_pHttpClient = NULL;
	}
}

int CHttpConnector::connect ()
{
	QLOG("connecting......\n");
	std::cout.flush();
	char buf[1024*4] = {0};
	std::string strEncodeReq = "";
	std::string strDecodedRes = "";
	GMutex.acquire();
	m_pHttpClient->m_PieceID = m_PieceID;
	std::string strReqMethord = gReqMethord::strGet;
	m_pHttpClient->bEncodeRequest(m_strURL, strEncodeReq, strReqMethord, m_RangeStart, m_RangeEnd);
	std::string strAddr = m_pHttpClient->m_strDomain;
	GMutex.release();
	if (NULL == strAddr.c_str())
		QLOG("Domain is NUll!\n");
	char str_port[] = "80";
	ACE_INET_Addr addr(str_port, strServerIp.c_str());

	int itmp = -1;
	int iDecodeRes = -1;
	int iCounter = 0;
	int i =0;
	bool pc = false;
	{
RE_Connect:	
		if (-1 == m_Connector.connect(m_SockStream, addr, &m_TimeOut))
		{
			iCounter += 10;
			m_SockStream.close();
			ACE_OS::sleep(iCounter);
			if (60 < iCounter)
			{
				QLOG("Timeout while connecting server,in thread connecting func(), will reconnect.\n");
				iCounter = 0;
			}
			addr = ACE_INET_Addr(str_port, strServerIp.c_str());
			goto RE_Connect;
		}
		i = m_SockStream.send_n(strEncodeReq.c_str(), strEncodeReq.length(), &m_TimeOut);
		if (i<=0)
		{
			iCounter += 10;
			m_SockStream.close();
			ACE_OS::sleep(iCounter);
			if (60 < iCounter)
			{
				QLOG("send EncodeReq to server failed.will reconnect.\n");
				iCounter = 0;
			}
			goto RE_Connect;
		}
		itmp = m_SockStream.recv(buf, sizeof(buf), &m_TimeOut);
		if (0 >= itmp)
		{
			iCounter += 10;
			ACE_OS::memset(buf, 0, sizeof(buf));
			m_SockStream.close();
			if (300 < iCounter)
			{
				iCounter = 0;
				QLOG("did not rev response from server,will reconnect. \n");
			}
			addr = ACE_INET_Addr(str_port, strServerIp.c_str());
			itmp = 0;
			goto RE_Connect;
		}
		//update range
		GMutex.acquire();
		if(m_pHttpClient->m_jieli_updaterange)
		{
			m_pHttpClient->UpdateRange(strEncodeReq, m_pHttpClient->m_offset, m_RangeEnd);
			m_pHttpClient->m_jieli_updaterange = false;
			m_SockStream.close();
			addr = ACE_INET_Addr(str_port, strServerIp.c_str());
			GMutex.release();
			goto RE_Connect;
		}
		GMutex.release();
		iDecodeRes = m_pHttpClient->DecodeResponse(buf, itmp);
		pc = m_pHttpClient->m_IsThisPieceComplete;
		if (pc)
		{
			return 0;
		}
		if (-1 == iDecodeRes)
		{
			m_pHttpClient->UpdateRange(strEncodeReq, /*m_RangeStart+*/m_pHttpClient->m_offset, m_RangeEnd);
			iCounter += 10;
			ACE_OS::memset(buf, 0, sizeof(buf));
			m_SockStream.close();
			ACE_OS::sleep(iCounter);
			if (60 < iCounter)
			{
				iCounter = 0;
				QLOG("request1: \n %s\n", strEncodeReq.c_str());
			}
			itmp = 0;
			goto RE_Connect;
		}
		if (0 == iDecodeRes);			

		while(1)
		{
			itmp = -1;
			ACE_OS::memset(buf, 0, sizeof(buf));
			pc = m_pHttpClient->m_IsThisPieceComplete;
			if (!pc)
			{
				itmp = m_SockStream.recv(buf, sizeof(buf), &m_TimeOut);
				if (0 >= itmp)
				{
					QLOG("did not receive reponse 1.will reconnect. \n");
					m_pHttpClient->UpdateRange(strEncodeReq, /*m_RangeStart+*/m_pHttpClient->m_offset, m_RangeEnd);
					QLOG("request2: \n %s\n", strEncodeReq.c_str());
					m_SockStream.close();

BBH:					
					itmp = 0;
					iCounter = 0;
					ACE_OS::memset(buf, 0, sizeof(buf));
					if (-1 == m_Connector.connect(m_SockStream, addr, &m_TimeOut))
					{		
						iCounter += 10;
						m_SockStream.close();
						ACE_OS::sleep(iCounter);
						if (60 <= iCounter)
						{
							iCounter = 0;
							QLOG("Timeout while connecting server in while(1) loop \n");
						}
						addr = ACE_INET_Addr(str_port, strServerIp.c_str());
						goto BBH;
					}
					i = m_SockStream.send_n(strEncodeReq.c_str(), strEncodeReq.length(), &m_TimeOut);
					if(i<=0)
					{
						iCounter += 10;
						m_SockStream.close();
						ACE_OS::sleep(iCounter);
						if (60 < iCounter)
						{
							iCounter = 0;
							QLOG("send EncodeReq to server failed in while(1) loop.will reconnect.\n");
						}
						goto BBH;

					}
					itmp = m_SockStream.recv(buf, sizeof(buf), &m_TimeOut);
					if (0 >= itmp)
					{
						m_pHttpClient->UpdateRange(strEncodeReq, /*m_RangeStart+*/m_pHttpClient->m_offset, m_RangeEnd);
						QLOG("request3: \n %s\n", strEncodeReq.c_str());
						iCounter +=10;
						m_SockStream.close();
						ACE_OS::sleep(iCounter);
						if (60 < iCounter)
						{
							iCounter = 0;
							QLOG("did not rev response from server in while(1) loop,will reconnect. \n");
						}
						addr = ACE_INET_Addr(str_port, strServerIp.c_str());
						itmp = 0;
						goto BBH;
					}
				}

				//update range
				GMutex.acquire();
				if(m_pHttpClient->m_jieli_updaterange)
				{
					m_pHttpClient->UpdateRange(strEncodeReq, m_pHttpClient->m_offset, m_RangeEnd);
					m_pHttpClient->m_jieli_updaterange = false;
					m_SockStream.close();
					addr = ACE_INET_Addr(str_port, strServerIp.c_str());
					GMutex.release();
					goto BBH;
				}
				GMutex.release();
				iDecodeRes = m_pHttpClient->DecodeResponse(buf, itmp);
				pc = m_pHttpClient->m_IsThisPieceComplete;
				if (pc)
					return 0;
				if (-1 == iDecodeRes)
				{
					m_pHttpClient->UpdateRange(strEncodeReq, /*m_RangeStart+*/m_pHttpClient->m_offset, m_RangeEnd);
					iCounter +=10;
					ACE_OS::memset(buf, 0, sizeof(buf));
					m_SockStream.close();
					ACE_OS::sleep(iCounter);
					if (60 < iCounter)
					{
						iCounter = 0;
						QLOG("request4: \n %s\n", strEncodeReq.c_str());
					}
					goto BBH;
				}
				if (0 == iDecodeRes);
			}
			else
			{
				break;
			}
		}//end of while(1)
		m_SockStream.close();
	}
	return 0;
}

int CHttpConnector::svc(void)
{
	connect();
	return 0;
}

bool CHttpDownloadTask::m_bFirstDisplay = false;

CHttpDownloadTask::CHttpDownloadTask(std::string strURL, int iPiecesNum)
{
	m_strURL = strURL;
	m_PieceNum = iPiecesNum;
	m_TaskID = 0;
	m_bTaskComplete = false;
	m_ThreadMap.clear();
	m_TimerID = 0;
	m_strFileName = "";
	time = 0;
	m_pre_offset = 0;
	m_StartTime = ACE_OS::gettimeofday();
	m_FileSize = 0;
	ACE_Reactor::instance()->register_handler(SIGPIPE, this);
}
int CHttpDownloadTask::LaunchTask()//get all threads ready
{
	ACE_INT64 iContentLen = 0;
	iContentLen = CHttpClient::HeadMethodForInfo(m_strURL, m_strFileName);
	m_FileSize = iContentLen;
	QLOG("iContentLen: %Q\n",iContentLen);
	if (0 == iContentLen)
	{
		QLOG("does not support HEAD methord.\n");
		bSupportHeadMethord = false;//if we can not get the file size,then assume it as chunked.
		m_PieceNum = 1;
	}

	std::string strLogFile = "."+m_strFileName;
	FILE *f = fopen(strLogFile.c_str(), "rb");
	if (f == NULL) 
	{ 
		QLOG("Can not open file %s, file does not exist.So add a new Task.\n", strLogFile.c_str());
		bHTTP_DP_GOON = false;
	} 
	else
	{
		fclose(f);
		bHTTP_DP_GOON = true;
	}

	if (bHTTP_DP_GOON)
	{
		QLOG("Continue downloading the file from dis-connect point:\n");

		if (m_FileSize/1024/1024)
			std::cout<<"FileSize: "<<(m_FileSize/1024/1024)<<" MB"<<std::endl;
		else if(m_FileSize/1024)
			std::cout<<"FileSize: "<<(m_FileSize/1024)<<" KB"<<std::endl;
		else
			std::cout<<"FileSize: "<<(m_FileSize)<<" Bytes"<<std::endl;
		QLOG("FileSize: %Q\n", m_FileSize);


		//read DP_GOON datas from log file to memory
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

		// [HEADflag][filename][url][filesize][ip][rate]|rangestart1:rangeend1|rangestart2:rangeend2...
		//get orig server support HeadMethord flag
		std::string::size_type iPos1 = strReadLog.find('[', 0);
		std::string::size_type iPos2 = strReadLog.find(']', iPos1);
		std::string strOrigHeadFlag = strReadLog.substr(iPos1+1, iPos2 - iPos1 -1 );


		//get filename
		iPos1 = strReadLog.find('[', iPos2);
		iPos2 = strReadLog.find(']', iPos1);
		std::string strFileNameLog = strReadLog.substr(iPos1+1, iPos2 - iPos1 -1 );

		//get url
		iPos1 = strReadLog.find('[', iPos2);
		iPos2 = strReadLog.find(']', iPos1);
		std::string strUrlLog = strReadLog.substr(iPos1+1, iPos2 - iPos1 -1 );


		//get filesize
		iPos1 = strReadLog.find('[', iPos2);
		iPos2 = strReadLog.find(']', iPos1);
		std::string strFileSizeLog = strReadLog.substr(iPos1+1, iPos2 - iPos1 -1 );
		ACE_INT64 iFileSizeLog = ACE_INT64_ATOI(strFileSizeLog.c_str(),strFileSizeLog.length());


		//get server ip
		iPos1 = strReadLog.find('[', iPos2);
		iPos2 = strReadLog.find(']', iPos1);
		std::string strIpFromLog = strReadLog.substr(iPos1+1, iPos2 - iPos1 -1 );
		QLOG("ServerIpGetFromLog: %s \n",strServerIp.c_str());

		//now ,we try to use ip got from log
		strServerIp = strIpFromLog;


		if (!bSupportHeadMethord)
		{
			QLOG("The Now server does not support HEAD methord, try to use ip get from log.");
			if(strOrigHeadFlag == "0")
			{
				QLOG("The now and orig servers do not support HEAD methord both,so start a new task from beginning.\n");
				goto newtask;
			}
			else if(strOrigHeadFlag == "1")
			{
				QLOG("The orig server support HEAD, so we try to use the orig ip/url/fileszie.\n");
				bSupportHeadMethord =1;
				strServerIp = strIpFromLog;
				m_FileSize = iFileSizeLog;
				m_strURL = strUrlLog;
			}
		}



		//get the rate
		iPos1 = strReadLog.find('[', iPos2);
		iPos2 = strReadLog.find(']', iPos1);
		std::string strRate = strReadLog.substr(iPos1+1, iPos2 - iPos1 -1 );
		gHTTP_Rate = ACE_INT64_ATOI(strRate.c_str(), strRate.length());

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
			--i;
		}

		QLOG("< Continued...... >\n");
		std::cout<<"< Continued...... >"<<std::endl;
		m_PieceNum = j;
		CHttpConnector* pConnector = NULL;
		ACE_Time_Value timeout(20,0);
		while(j>0)
		{
			pConnector = new CHttpConnector(m_strURL, timeout,
					PairArray[j-1].RangeStart, PairArray[j-1].RangeEnd, j-1, bSupportHeadMethord, m_PieceNum, m_strFileName);
			if (NULL != pConnector)
			{
				m_ThreadMap.insert(std::map<int, CHttpConnector*>::value_type(j-1, pConnector));
				while(-1 == pConnector->activate(THR_NEW_LWP|THR_JOINABLE|THR_INHERIT_SCHED|THR_SUSPENDED))
				{
					ACE_OS::sleep(1);
					std::cout<<"activate thread failed!maybe the thread number is too large!"<<std::endl;
				}
			}
			else
			{
				std::cout<<"alloc memory failed!"<<std::endl;
			}
			--j;
		}

		Start();
		QLOG("HttpTask Launched!\n");

		return m_PieceNum;
	}

newtask:
	if (true == bSupportHeadMethord)
	{
		if (m_FileSize/1024/1024)
			std::cout<<"FileSize: "<<(m_FileSize/1024/1024)<<" MB"<<std::endl; 
		else if(m_FileSize/1024)
			std::cout<<"FileSize: "<<(m_FileSize/1024)<<" KB"<<std::endl;
		else
			std::cout<<"FileSize: "<<(m_FileSize)<<" Bytes"<<std::endl;
		if(!CFileStore::CreateFile(m_strFileName, m_FileSize))
			exit(1);
		QLOG("FileSize: %Q\n", m_FileSize);
	}
	else
	{
		QLOG("Can not get file size, so chunked processing.....");
		CFileStore::CreateFile(m_strFileName, 0);
	}

	ACE_INT64 iRangeSize = (iContentLen-1)/m_PieceNum;
	ACE_INT64 iLeft = (iContentLen-1)%m_PieceNum;
	unsigned int iRangeStart = 1;
	unsigned int iRangeEnd = 0;
	CHttpConnector* pConnector = NULL;
	ACE_Time_Value timeout(20,0);

	ACE_INT64 i=0;
	if (iContentLen > 0)
	{
		for (i=0;i < m_PieceNum-1;)
		{
			pConnector = new CHttpConnector(m_strURL, timeout, 
					i*iRangeSize, (i+1)*iRangeSize-1, i, bSupportHeadMethord, m_PieceNum, m_strFileName);
			if (NULL != pConnector)
			{
				m_ThreadMap.insert(std::map<int, CHttpConnector*>::value_type(i, pConnector));
				while(-1 == pConnector->activate(THR_NEW_LWP|THR_JOINABLE|THR_INHERIT_SCHED|THR_SUSPENDED))
				{
					ACE_OS::sleep(1);
					std::cout<<"activate thread failed!maybe the thread number is too large!"<<std::endl;
				}
				++i;
			}
			else
			{
				std::cout<<"alloc memory failed!"<<std::endl;
			}
		}	
		pConnector = new CHttpConnector(m_strURL, timeout, 
				i*iRangeSize, iContentLen-1, i, bSupportHeadMethord, m_PieceNum, m_strFileName);
		if (NULL != pConnector)
		{
			m_ThreadMap.insert(std::map<int, CHttpConnector*>::value_type(i, pConnector));
			while(-1 == pConnector->activate(THR_NEW_LWP|THR_JOINABLE|THR_INHERIT_SCHED|THR_SUSPENDED))
			{
				ACE_OS::sleep(1);
				std::cout<<"activate thread failed!maybe the thread number is too large!"<<std::endl;
			}
		}
		else
		{
			std::cout<<"alloc memory failed!"<<std::endl;
		}

	}
	else
	{
		m_PieceNum = 1;
		pConnector = new CHttpConnector(m_strURL, timeout, 0, 0, 0, bSupportHeadMethord, 1, m_strFileName);
		if (NULL != pConnector)
		{
			m_ThreadMap.insert(std::map<int, CHttpConnector*>::value_type(i, pConnector));
			while(-1 == pConnector->activate(THR_NEW_LWP|THR_JOINABLE|THR_INHERIT_SCHED|THR_SUSPENDED))
			{
				ACE_OS::sleep(1);
				std::cout<<"activate thread failed!maybe the thread number is too large!"<<std::endl;
			}
		}
		else
		{
			std::cout<<"alloc memory failed!"<<std::endl;
		}
	}

	Start();
	QLOG("HttpTask Launched!\n");
	return m_PieceNum;
}

bool CHttpDownloadTask::Start()
{
	std::map<int, CHttpConnector*>::iterator pos;
	for (pos = m_ThreadMap.begin();pos!=m_ThreadMap.end();++pos)
	{
		pos->second->resume();
	}
	return true;
}
bool CHttpDownloadTask::Resume()
{
	std::map<int, CHttpConnector*>::iterator pos;
	for (pos = m_ThreadMap.begin();pos!=m_ThreadMap.end();++pos)
	{
		pos->second->resume();
	}
	return true;
}
bool CHttpDownloadTask::Suspend()
{
	std::map<int, CHttpConnector*>::iterator pos;
	for (pos = m_ThreadMap.begin();pos!=m_ThreadMap.end();++pos)
	{
		pos->second->suspend();
	}
	return true;
}
bool CHttpDownloadTask::Delete()
{
	return true;
}
bool CHttpDownloadTask::Stop()
{
	return true;
}

bool CHttpDownloadTask::GetSpeedAndRate(double &speed, int &rate)
{
	GMutex.acquire();
	//get current time
	ACE_Time_Value cur = ACE_OS::gettimeofday();
	ACE_Time_Value tmp = cur - m_StartTime;
	m_StartTime = cur;
	std::map<int, CHttpConnector*>::iterator pos;
	float percent = 0.000000;
	unsigned int offset = 0;
	for (pos = m_ThreadMap.begin(); pos != m_ThreadMap.end(); ++pos)
	{
		percent += (pos->second)->m_pHttpClient->m_fPrecent;
		offset += (pos->second)->m_pHttpClient->m_offset/1024;	
	}
	int perc = int((percent/m_PieceNum)*100);
	rate = perc;

	float sp = 0.0;
	sp = (offset-m_pre_offset)/((tmp.msec()/1000)>0?(tmp.msec()/1000):1 );
	speed = sp;
	m_pre_offset = offset;

	bool bTmp =  false;
	for (pos = m_ThreadMap.begin(); pos != m_ThreadMap.end(); ++pos)
	{
		bTmp = (pos->second)->m_pHttpClient->m_IsThisPieceComplete;
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
		m_bTaskComplete = true;
	}
	GMutex.release();

	return true;
}

ACE_INT64 CHttpDownloadTask::GetFileSize()
{
	return m_FileSize;
}

int CHttpDownloadTask::handle_signal(int signum, siginfo_t*,ucontext_t *)
{
	QLOG("HTTP SIGPIPE\n");
	return 0;
}

//this timer is for updating speed and rate
//1 ms
int CHttpDownloadTask::handle_timeout(const ACE_Time_Value & current_time, const void *)
{
	GMutex.acquire();
	//the interval is 100 ms
	time+=0.1;
	std::map<int, CHttpConnector*>::iterator pos;
	float percent = 0;
	unsigned int offset = 0;
	for (pos = m_ThreadMap.begin(); pos != m_ThreadMap.end(); ++pos)
	{

		percent += (pos->second)->m_pHttpClient->m_fPrecent;
		offset += ((pos->second)->m_pHttpClient->m_offset - (pos->second)->m_pHttpClient->m_RangeStart) / 1024;

	}
	int perc = int((percent/m_PieceNum)*100);
	if (false == CHttpDownloadTask::m_bFirstDisplay)
	{
		std::cout<<std::setw(6)<<std::right<<"\r=> %"<<perc<<" | speed: "<<std::setw(10)<<std::right<<float(offset-m_pre_offset)/(float)(0.1)<<"KB/S |" \
			<<" arg speed: "<<std::setw(10)<<std::right<<std::setprecision(4)<<offset/time<<"KB/S |"  \
			<<" time: "<<std::setw(6)<<std::right<<time<<" s";
		CHttpDownloadTask::m_bFirstDisplay= true;
		m_fSpeed = (offset-m_pre_offset)/(float)0.1;
		m_iPerc = perc;
	}
	else
	{
		std::cout<<std::setw(6)<<std::right<<"\r=> %"<<perc<<" | speed: "<<std::setw(10)<<std::right<<(offset-m_pre_offset)/(float)(0.1)<<"KB/S |" \
			<<" arg speed: "<<std::setw(10)<<std::right<<std::setprecision(4)<<offset/time<<"KB/S |" \
			<<" time: "<<std::setw(6)<<std::right<<time<<" s";
		m_fSpeed = (offset-m_pre_offset)/(float)0.1;
		m_iPerc = perc;
	}
	std::cout.flush();
	m_pre_offset = offset;

	bool bTmp =  false;
	for (pos = m_ThreadMap.begin(); pos != m_ThreadMap.end(); ++pos)
	{
		bTmp = (pos->second)->m_pHttpClient->m_IsThisPieceComplete;
		if (true == bTmp)
		{
			;
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
	GMutex.release();
	return 1;
}

CHttpDownloadTask::~CHttpDownloadTask()
{
	//GMutex.acquire();
	std::map<int, CHttpConnector*>::iterator pos;
	for (pos = m_ThreadMap.begin(); pos != m_ThreadMap.end();)
	{
		if(NULL != pos->second)
		{
			delete pos->second;
			pos->second = NULL;
		}
		m_ThreadMap.erase(pos);
		pos = m_ThreadMap.begin();	
	}
	//GMutex.release();
}

CHttpDownloadTaskManager::CHttpDownloadTaskManager()
{
	m_iTaskID = 0;
	m_TaskMap.clear();
}

int CHttpDownloadTaskManager::init ()
{
	return 0;
}

long CHttpDownloadTaskManager::AddTask(std::string strURL, int iPiecesNum)
{
	if (strURL[strURL.length()-1] == '/')
		CHttpDownloadTask* pHttpTask = new CHttpDownloadTask(strURL, 1);
	CHttpDownloadTask* pHttpTask = new CHttpDownloadTask(strURL, iPiecesNum);
	//ACE_Time_Value initDelay(0,500*1000);
	//ACE_Time_Value interval(0,500*1000);
	//pHttpTask->m_TimerID = ACE_Reactor::instance()->schedule_timer(pHttpTask, 0, initDelay, interval);
	pHttpTask->LaunchTask();

	++m_iTaskID;
	pHttpTask->m_TaskID = m_iTaskID;
	m_TaskMap.insert(std::map<long, CHttpDownloadTask*>::value_type(m_iTaskID, pHttpTask));
	QLOG("Task added!\n");
	return m_iTaskID;
}

bool CHttpDownloadTaskManager::StartTask(long lTaskID)
{
	CHttpDownloadTask* p = GetTask(lTaskID);
	p->Start();
	return true;
}
bool CHttpDownloadTaskManager::DeleteTask(long lTaskID)
{
	CHttpDownloadTask* p = GetTask(lTaskID);
	p->Delete();
	return true;
}
bool CHttpDownloadTaskManager::SuspendTask(long lTaskID)
{
	CHttpDownloadTask* p = GetTask(lTaskID);
	p->Suspend();
	return true;
}
bool CHttpDownloadTaskManager::ResumeTask(long lTaskID)
{
	CHttpDownloadTask* p = GetTask(lTaskID);
	p->Resume();
	return true;
}
bool CHttpDownloadTaskManager::StopTask(long lTaskID)
{
	CHttpDownloadTask* p = GetTask(lTaskID);
	p->Stop();
	return true;
}
bool CHttpDownloadTaskManager::GetSpeedAndRate(long lTaskID, double &speed, int &rate)
{
	CHttpDownloadTask* p = GetTask(lTaskID);
	p->GetSpeedAndRate(speed, rate);
	return true;
}

ACE_INT64 CHttpDownloadTaskManager::GetFileSize(long lTaskID)
{
	CHttpDownloadTask* p = GetTask(lTaskID);
	return p->GetFileSize();
}

CHttpDownloadTask* CHttpDownloadTaskManager::GetTask(long lTaskID)
{
	CHttpDownloadTask* p = NULL;
	std::map<long, CHttpDownloadTask*>::iterator pos;
	pos = m_TaskMap.find(lTaskID);
	p = pos->second;
	return p;
}

//this timer is for checking if the task is completed.
int CHttpDownloadTaskManager::handle_timeout(const ACE_Time_Value & current_time, const void *)
{
	//disconnect-point log
	std::string strDP_Log = "";


	//update speed here, we use only 1 timer
	//float t_piece = 0.1;//1 ms
	float t_piece = 0.5;//5 ms
	//float t_piece = 1;//1 s
	std::map<long, CHttpDownloadTask*>::iterator pos;
	CHttpConnector * pCon = NULL;
	for (pos = m_TaskMap.begin(); pos != m_TaskMap.end(); ++pos)
	{
		int perc = 0;
		ACE_INT64 iPartFileSize = 0;
		pos->second->time+=t_piece;
		std::map<int, CHttpConnector*>::iterator pos2;
		float percent = 0;
		//unsigned int offset = 0;
		ACE_INT64 offset = 0;
		for (pos2 = pos->second->m_ThreadMap.begin(); pos2 != pos->second->m_ThreadMap.end(); ++pos2)
		{
			if (!pos2->second->m_pHttpClient->m_IsThisPieceComplete)
			{
				strDP_Log += "|" + ACE_INT64_ITOA(pos2->second->m_pHttpClient->m_offset, 10) + ":" \
					      + ACE_INT64_ITOA(pos2->second->m_pHttpClient->m_RangeEnd, 10);
			}

			if(bHTTP_DP_GOON)
			{
				iPartFileSize += pos2->second->m_pHttpClient->m_fPrecent * \
						 (pos2->second->m_pHttpClient->m_RangeEnd-pos2->second->m_pHttpClient->m_RangeStart+1);
			}
			else
			{
				percent += (pos2->second)->m_pHttpClient->m_fPrecent;
			}
			offset += (pos2->second)->m_pHttpClient->m_offset - (pos2->second)->m_pHttpClient->m_RangeStart;
		}
		offset += overloadpiece_len;
		percent += overloadpiece_percent;

		GMutex.acquire();
		if (pos->second->m_PieceNum == Complete_Num)
		{
			pos->second->m_bTaskComplete = true;
		}
		else
		{
			//try to implement 'jieli',try to make it at least there are 2 or 5 threads to download
			if ((5 > pos->second->m_PieceNum && pos->second->m_PieceNum > 1 
						&& pos->second->m_PieceNum - Complete_Num == 1 && bSupportHeadMethord)
					|| (pos->second->m_PieceNum > 5 
						&& pos->second->m_PieceNum - Complete_Num < 5 && bSupportHeadMethord))
			{
				ACE_INT64 itmp = 0;
				bool bFindCompleteFirst = false;
				bool bFindNonCompleteFirst = false;
				bool bUpdatePos = false;
				pCon = NULL;
				std::map<int, CHttpConnector*>::iterator pos3 = pos->second->m_ThreadMap.end();
				std::map<int, CHttpConnector*>::iterator pos4 = pos->second->m_ThreadMap.end();
				for (pos2 = pos->second->m_ThreadMap.begin(); pos2 != pos->second->m_ThreadMap.end(); ++pos2)
				{
					//find a completed piece
					if (pos2->second->m_pHttpClient->m_IsThisPieceComplete)
					{
						pCon = pos2->second;
						if(bFindNonCompleteFirst)
						{
							pos4 = pos2;
							pos2 = pos3;
							bFindNonCompleteFirst = false;
							pos3 = pos->second->m_ThreadMap.end();
							bUpdatePos = true;
							goto start_new_thread;
						}
					}
					else if (NULL != pCon)//find a non-completed piece
					{
start_new_thread:
						itmp = pos2->second->m_RangeEnd - pos2->second->m_pHttpClient->m_offset+1;
						if(itmp > (ACE_INT64)(256*1024))//make it bigger, 256KB
						{
							--Complete_Num;
							//correct the speed datas
							overloadpiece_len += pCon->m_pHttpClient->m_offset-pCon->m_RangeStart;
							overloadpiece_percent += pCon->m_pHttpClient->m_fPrecent;

							//update the jieli thread
							pCon->m_pHttpClient->m_IsThisPieceComplete = false;
							pCon->m_pHttpClient->m_RangeStart = \
											    pos2->second->m_pHttpClient->m_offset + itmp/2;
							pCon->m_pHttpClient->m_offset = pCon->m_pHttpClient->m_RangeStart;
							pCon->m_pHttpClient->m_RangeEnd = pos2->second->m_RangeEnd;
							pCon->m_pHttpClient->m_fPrecent = 0.0;
							pCon->m_RangeStart = pCon->m_pHttpClient->m_RangeStart;
							pCon->m_RangeEnd = pCon->m_pHttpClient->m_RangeEnd;
							QLOG("NEW TASK: RangeStart: %Q  | RangeEnd: %Q \n", pCon->m_RangeStart, pCon->m_RangeEnd);

							//update the jieli-ed thread
							pos2->second->m_RangeEnd = pCon->m_RangeStart - 1;
							pos2->second->m_pHttpClient->m_RangeEnd = pos2->second->m_RangeEnd;
							pos2->second->m_pHttpClient->m_fPrecent \
								= float(pos2->second->m_pHttpClient->m_offset-pos2->second->m_RangeStart) \
								/(pos2->second->m_RangeEnd - pos2->second->m_RangeStart + 1);
							//update the range sent to server
							pos2->second->m_pHttpClient->m_jieli_updaterange = true;
							QLOG("jieli-ed TASK: RangeStart: %Q  | RangeEnd: %Q \n", \
									pos2->second->m_RangeStart, pos2->second->m_RangeEnd);

							itmp = 0;
							break;
						}// > 256K
						else
						{
							itmp = 0;
							//try to find another uncomplated piece
							if (bUpdatePos)
							{
								pos2 = pos4;// re-assign to it's orig value
								pos4 = pos->second->m_ThreadMap.end();
								bUpdatePos = false;
							}
						}
					}// pCon != NULL
					else
					{
						//record the positon of the uncompleted thread
						pos3 = pos2;
						bFindNonCompleteFirst = true;
					}
				}//for pos2
			}
		}//else

		GMutex.release();


		//active the jieli thread
		if (NULL != pCon && pos2 != pos->second->m_ThreadMap.end())
		{
			while(-1 == pCon->activate(THR_NEW_LWP|THR_JOINABLE \
						|THR_INHERIT_SCHED|THR_SUSPENDED))
			{
				std::cout<<"activate thread failed!maybe the thread number \
					is too large!"<<std::endl;
			}
			pCon->resume();
			QLOG("<<<Start a jieli thread successfully>>>>>>>.\n");
			pCon = NULL;
		}




		if (bHTTP_DP_GOON)
		{
			//we need to re-get the global rate number, 
			//but it's hard to get the accurate data,so at the end we turn to decide if the task is completed.
			perc = ((double)iPartFileSize/pos->second->m_FileSize)*100 + gHTTP_Rate;
			if(pos->second->m_bTaskComplete)
				perc = 100;
		}
		else
		{
			perc = int(((float)offset/pos->second->m_FileSize)*100);
		}
		if (false == CHttpDownloadTask::m_bFirstDisplay)
		{
			std::cout<<std::setw(6)<<std::right<<"\r=> %"<<perc \
				<<" |speed: "<<std::setw(10)<<std::right<<float(offset-pos->second->m_pre_offset)/t_piece/1024<<"KB/S |" \
				<<"arg speed: "<<std::setw(10)<<std::setiosflags(ios::fixed)<<std::right<<std::setprecision(1)<<offset/pos->second->time/1024<<"KB/S |"  \
				<<"time: "<<std::setw(6)<<std::right<<pos->second->time<<" s";
			CHttpDownloadTask::m_bFirstDisplay= true;
			pos->second->m_fSpeed = (offset-pos->second->m_pre_offset)/t_piece;
			pos->second->m_iPerc = perc;
		}
		else
		{
			std::cout<<std::setw(6)<<std::right<<"\r=> %"<<perc \
				<<" |speed: "<<std::setw(10)<<std::right<<(offset-pos->second->m_pre_offset)/t_piece/1024<<"KB/S |" \
				<<"arg speed: "<<std::setw(10)<<std::setiosflags(ios::fixed)<<std::right<<std::setprecision(1)<<offset/pos->second->time/1024<<"KB/S |" \
				<<"time: "<<std::setw(6)<<std::right<<pos->second->time<<" s";
			pos->second->m_fSpeed = (offset-pos->second->m_pre_offset)/t_piece;
			pos->second->m_iPerc = perc;
		}
		std::cout.flush();
		pos->second->m_pre_offset = offset;


		//update the DP log datas.
		//the log format:
		// [HEADflag][filename][url][filesize][ip][rate]|rangestart1:rangeend1|rangestart2:rangeend2...
		strDP_Log = "[" + ACE_INT64_ITOA(perc, 10) + "]" + strDP_Log;
		strDP_Log = "[" + strServerIp + "]" + strDP_Log;
		strDP_Log = "[" + ACE_INT64_ITOA(pos->second->m_FileSize, 10) + "]" + strDP_Log;
		strDP_Log = "[" + pos->second->m_strURL + "]" + strDP_Log;
		strDP_Log = "[" + pos->second->m_strFileName + "]" + strDP_Log;
		if(bSupportHeadMethord)
			strDP_Log = "[1]" + strDP_Log;
		else
			strDP_Log = "[0]" + strDP_Log;
		//CFileStore::CreateFile("."+pos->second->m_strFileName, 0);
		//remove(std::string("."+pos->second->m_strFileName).c_str());
		pos->second->m_FileStore.StoreData("."+pos->second->m_strFileName, 0, " ",1);
		pos->second->m_FileStore.StoreData("."+pos->second->m_strFileName, 0, (char*)strDP_Log.c_str(), strDP_Log.length());

		//next, check if the task is completed.
		if (pos->second->m_bTaskComplete)
		{
			ACE_Reactor::instance()->cancel_timer (this);
			//delete the dp log datas
			std::string strTmpFileName = "." + pos->second->m_strFileName;
			while(0 != remove(strTmpFileName.c_str()))
				;

			delete pos->second;
			pos->second = NULL;
			m_TaskMap.clear();
			std::cout<<"   Complete!"<<std::endl;
			QLOG("Complete!\n\n");
			exit(0);
		}

	}
	return 1;
}

CHttpDownloadTaskManager::~CHttpDownloadTaskManager()
{
	std::map<long, CHttpDownloadTask*>::iterator pos;
	for (pos = m_TaskMap.begin(); pos != m_TaskMap.end();)
	{
		delete pos->second;
		pos->second = NULL;
		m_TaskMap.erase(pos);
		pos = m_TaskMap.begin();
	}
}

#endif //_HTTP_DECODE_CPP

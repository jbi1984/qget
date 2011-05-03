#include "func.h"

ACE_INT64 ACE_INT64_ATOI(const char * input, int len)
{
        ACE_INT64 ret_value = 0;
        int i  =0;
        while (len>0)
        {
                //if (input[len-1]>='0' && input[len-1]<='9')   
                //{
                        ret_value += (input[len-1]-'0') * pow(10,i);
                        ++i; --len;
                //}
                /*/else
                {
                        std::cout<<"input error for func ACE_INT64_ATOI"<<std::endl;
                        exit(1);
                }*/
        }
        return ret_value;
}


std::string ACE_INT64_ITOA(ACE_INT64 iNum, char *buf, int index)
{
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
        return strRet;
}

std::string ACE_INT64_ITOA(ACE_INT64 iNum, int index)
{
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
	return strRet;
}

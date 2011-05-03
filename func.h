/*
 * 2011 -2021 ,All rights reserved.
 * Contact: heguanbo@gmail.com/gjhe@novell.com
 */
#include <string>
#include <cmath>
#include "ace/Basic_Types.h"
#include <iostream>
#include <map>
#include <algorithm>
#include <iomanip>


ACE_INT64 ACE_INT64_ATOI(const char * input, int len);
std::string ACE_INT64_ITOA(ACE_INT64 iNum, char *buf, int index);
std::string ACE_INT64_ITOA(ACE_INT64 iNum, int index);

#include "qstring.h"
void str_trim_crlf(char* str)
{
	char* p = &str[strlen(str)-1];
	while(*p == '\r' || *p == '\n')
		*p-- = '\0';
}
void str_split(const char* str,char* left,char* right,char c)
{
	char* p = strchr(str,c);
	if(p==NULL)
		strcpy(left,str);
	else
	{
		strncpy(left,str,p-str);//不包括c所以不用加1
		strcpy(right,p+1);
	}
}
int str_all_space(const char* str)
{
	while(*str)
	{
		if(!isspace(*str))
			return 0;
		str++;
	}
	return 1;
}
void str_to_upper(char* str)
{
	while(*str)
	{
		*str = toupper(*str);//将自己转换后的值赋给自己
		str++;
	}

}
long long str_to_longlong(const char* str)//%lld查看longlong形
{
	/*系统有long long atoll(const char *nptr)方法
	 * 但是并不是所有系统都能用,可移植性差
	 * return atoll(str);
	*/
	long long result = 0;
	long long mult = 1;
	int len  = strlen(str);
	int i;
	if(len > 15)
		return 0;//太大long long 也放不下
	for(i = len-1; i >= 0; i--)//如果i是unsigned int则会死循环，因为i=0时减一又为最大值
	{
		char ch = str[i];
		if(ch < '0' || ch > '9')
			return 0;
		long long val;
		val = ch - '0';
		val *= mult;
		mult *= 10;
		result += val;
	}
	return result;
}
unsigned int str_oct_to_uint(const char* str)
{
	//方法一，低位开始与上个方法类似
	//方法二：
	unsigned int result = 0;
	int first_non_zero_digit;
	while(*str)
	{
		int digit = *str;
		if(!isdigit(digit) || digit > '7')
			break;
		if(digit != '0')
			first_non_zero_digit = 1;
		if(first_non_zero_digit)
		{
			result *= 8;//左移3位等于乘以8
			result += digit - '0';
		}
		str++;
	}
	return result;
}

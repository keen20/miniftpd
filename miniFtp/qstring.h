#ifndef _QSTRING_H_
#define _QSTRING_H_
#include "common.h"
void str_trim_crlf(char* str);//去除/r/n
void str_split(const char* str,char* left,char* right,char c);//分割字符串
int str_all_space(const char* str);//判断是否全是空格
void str_to_upper(char* str);
long long str_to_longlong(const char* str);//字符串转换为longlong整形(八字节)
unsigned int str_oct_to_uint(const char* str);//将字符串(八进制)转换成无符号整数
#endif

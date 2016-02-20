#ifndef _COMMON_H_
#define _COMMON_H_
#include<unistd.h>
#include <stdlib.h>
#include<stdio.h>
#include<errno.h>
#include <signal.h>
#include<string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>//与gethostbyname有关
#include <pwd.h>
#include <shadow.h>
#include <crypt.h>
#include <time.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/sendfile.h>
#define ERR_EXIT(m)\
	do {perror(m);\
	exit(EXIT_FAILURE);\
	} while(0)
#define MAX_COMMAND_LINE 1024 //每个命令行最大字节为1024
#define MAX_COMMAND 32  //每个命令最大是32
#define MAX_ARG 1024//命令所带参数最大是 1024
#define MINIFTPD "/etc/miniftpd.conf"
#define PASV_MODE_SERVER_IP "192.168.43.131"//pasv模式的服务器ip地址
#endif

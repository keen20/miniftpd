#ifndef _SESSION_H_
#define _SESSION_H_
#include "common.h"
typedef struct session
{
	//控制连接
		uid_t uid;
		int conn;//已连接套接字
		char cmdline[MAX_COMMAND_LINE];
		char cmd[MAX_COMMAND];
		char args[MAX_ARG];
	//数据连接
		struct sockaddr_in* port_addr;
		int pasv_listen_fd;
		int data_fd;//数据套接字
	//限速
		unsigned int bw_upload_rate_max;
		unsigned int bw_download_rate_max;
		long bw_transfer_start_sec;//开始传输时间
		long bw_transfer_start_usec;
	//父子进程通道
		int parent_fd;//父进程使用的通道
		int child_fd;//子进程使用的通道
	//FTP协议状态
		int is_ascii;
		long long restart_position;//断点续传
		char* rnfr_mark;//重命名之前状态保存
}SESSION;
void begin_session(SESSION* sess);

#endif

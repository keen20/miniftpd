/*
 * 因为FTP服务器是需要root用户启动的
 * 因此需要先判断是否是root用户
 */
#include "sysutil.h"
#include "session.h"
#include "qstring.h"
#include "turnable.h"
#include "parseconf.h"
#include "ftpproto.h"
int main(void)
{

	daemon(0,0);
	parseconf_load_file(MINIFTPD);
	/*printf("turnable_pasv_enable=[%d]\n",turnable_pasv_enable);
	printf("turnable_port_enable=[%d]\n",turnable_port_enable);
	printf("turnable_listen_port=[%u]\n",turnable_listen_port);
	printf("turnable_max_clients=[%u]\n",turnable_max_clients);
	printf("turnable_max_per_ip=[%u]\n",turnable_max_per_ip);
	printf("turnable_accept_timeout=[%u]\n",turnable_accept_timeout);
	printf("turnable_connect_timeout=[%u]\n",turnable_connect_timeout);
	printf("turnable_idle_session_timeout=[%u]\n",turnable_idle_session_timeout);
	printf("turnable_data_connection_timeout=[%u]\n",turnable_data_connection_timeout);
	printf("turnable_local_umask=[0%o]\n",turnable_local_umask);
	printf("turnable_upload_max_rate=[%u]\n",turnable_upload_max_rate);
	printf("turnable_download_max_rate=[%u]\n",turnable_download_max_rate);
	if(turnable_listen_address == NULL)
		printf("NULL\n");
	else
		printf("turnable_listen_address = %s\n",turnable_listen_address);*/
	if(getuid() != 0)
	{
		fprintf(stderr,"must be started by root\n");
		exit(EXIT_FAILURE);
	}
	//初始化一个会话结构体
	SESSION sess =
	{
		//控制连接
			0,-1,"","","",
		//数据连接
			NULL,-1,-1,
		//限速
			0,0,0,0,
		//父子通道
			-1,-1, //为什么取-1，因为非负整形的无效数据就是负值
		//是否是ASCII模式
			0,0,NULL
	};
	sess.bw_download_rate_max = turnable_download_max_rate;
	sess.bw_upload_rate_max = turnable_upload_max_rate;
	//printf("sess.bw_upload_rate_max=[%u]\n",sess.bw_upload_rate_max);
	//printf("sess.bw_download_rate_max=[%u]\n",sess.bw_download_rate_max);
	signal(SIGCHLD,SIG_IGN);//子进程退出自动向父进程发送SIGCHID
	int listenfd = tcp_server(NULL,10000);
	int conn;
	pid_t pid;
	while(1)
	{
		conn = accept_timeout(listenfd,NULL,0);
		if(conn == -1)
			ERR_EXIT("accept_timeout");
		pid = fork();
		if(pid < 0)
			ERR_EXIT("fork");
		else if(pid == 0)//子进程负责执行业务会话
		{
			close(listenfd);
			sess.conn = conn;
			begin_session(&sess);
		}else//父进程负责监听连接请求
		{
			close(conn);
		}
	}
	return 0;
}

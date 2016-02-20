#include "privateparent.h"
#include "privatesock.h"
#include "turnable.h"
static void privop_sock_get_data_sock(SESSION* sess);
static void privop_pasv_active(SESSION* sess);
static void privop_pasv_listen(SESSION* sess);
static void privop_pasv_accept(SESSION* sess);
void handle_parent(SESSION* sess)
{
	/*将父进程改为nobody进程
	 * 因为nobody进程的目的就是辅助服务进程执行一些没有权限的操作
	 * 一定要在父进程进程分支上做，不然就会将两个进程都变为nobody*/

	struct passwd* pw = getpwnam("nobody"); //获取nobody用户
	if (pw == NULL)
		return;
	//一定要先改组ID，再改UID，以免没有权限
	if (setegid(pw->pw_gid) < 0)
		ERR_EXIT("setegid");
	if (seteuid(pw->pw_uid) < 0)
		ERR_EXIT("seteuid");
	/*if (setegid(0) < 0)
		ERR_EXIT("setegid");
	if (seteuid(0) < 0)
		ERR_EXIT("seteuid");*/
	char cmd;
	while (1)
	{
		//当服务进程退出就会关闭相关套接口，则priv_sock_get_cmd就会返回0，然后退出
		cmd = priv_sock_get_cmd(sess->parent_fd);
		switch(cmd)
		{
			case PRIV_SOCK_GET_DATA_SOCK:
				privop_sock_get_data_sock(sess);
				break;
			case PRIV_SOCK_PASV_ACTIVE:
				privop_pasv_active(sess);
				break;
			case PRIV_SOCK_PASV_LISTEN:
				privop_pasv_listen(sess);
				break;
			case PRIV_SOCK_PASV_ACCEPT:
				privop_pasv_accept(sess);
				break;
		}
	}
}

static void privop_sock_get_data_sock(SESSION* sess)
{
	unsigned short port = (unsigned short)priv_sock_get_int(sess->parent_fd);
	char ip[16] = {0};
	priv_sock_recv_buf(sess->parent_fd,ip,sizeof(ip));
	//nobody创建数据连接
	struct sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);
	/*创建socket
	 * bind 20
	 * connect
	 */

	int fd = tcp_client(0);
	/*struct passwd* pw = getpwnam("nobody"); //获取nobody用户
	if (pw == NULL)
		return;
	//一定要先改组ID，再改UID，以免没有权限
	if (setegid(pw->pw_gid) < 0)
		ERR_EXIT("setegid");
	if (seteuid(pw->pw_uid) < 0)
		ERR_EXIT("seteuid");*/
	if(fd == -1)
	{
		priv_sock_send_cmd(sess->parent_fd,PRIV_SOCK_RESULT_BAD);
		return ;
	}

	if (connect_timeout(fd, &addr, turnable_connect_timeout) < 0)
	{
		close(fd);
		priv_sock_send_cmd(sess->parent_fd,PRIV_SOCK_RESULT_BAD);
		return ;
	}
	//成功
	priv_sock_send_cmd(sess->parent_fd,PRIV_SOCK_RESULT_OK);
	//发送fd给服务进程,为什么不直接sess->data_fd = fd;因为sess->port_addr数据结构属于服务进程
	priv_sock_send_fd(sess->parent_fd,fd);
	close(fd);
}
static void privop_pasv_active(SESSION* sess)
{
	int active;
	if(sess->pasv_listen_fd != -1)
	{
		active = 1;
	}
	else
	{
		active = 0;
	}
	priv_sock_send_int(sess->parent_fd,active);
}
static void privop_pasv_listen(SESSION* sess)
{
	char ip[16] = PASV_MODE_SERVER_IP;
	sess->pasv_listen_fd = tcp_server(ip, 0);	//0表示主机动态分配端口
	//printf("返回后fd是：%d\n",sess->pasv_listen_fd);
	struct sockaddr_in seraddr;
	memset(&seraddr, 0, sizeof(seraddr));
	socklen_t seraddrlen = sizeof(seraddr);
	if (getsockname(sess->pasv_listen_fd, (struct sockaddr *) &seraddr, &seraddrlen) < 0)
	{
		ERR_EXIT("getsockname");
	}
	unsigned short port = ntohs(seraddr.sin_port);
	priv_sock_send_int(sess->parent_fd,(int)port);
	//priv_sock_send_int(sess->parent_fd,(int)sess->pasv_listen_fd);
}
static void privop_pasv_accept(SESSION* sess)
{
	int fd;
	fd = accept_timeout(sess->pasv_listen_fd,NULL,turnable_accept_timeout);
	//关闭监听套接字，以后接到命令重新监听连接
	close(sess->pasv_listen_fd);
	sess->pasv_listen_fd = -1;
	if(fd == -1)
	{
		priv_sock_send_result(sess->parent_fd,PRIV_SOCK_RESULT_BAD);
		return;
	}
	priv_sock_send_result(sess->parent_fd,PRIV_SOCK_RESULT_OK);
	priv_sock_send_fd(sess->parent_fd,fd);
	close(fd);
}

#include "privatesock.h"
#include "common.h"
#include "sysutil.h"
void priv_sock_init(SESSION* sess)
{
	int socketfds[2];
	if (socketpair(PF_UNIX, SOCK_STREAM, 0, socketfds) < 0)
		ERR_EXIT("socketpair");
	sess->parent_fd = socketfds[0];
	sess->child_fd = socketfds[1];
}
void priv_sock_close(SESSION* sess)
{
	if (sess->parent_fd != -1)
	{
		close(sess->parent_fd);
		sess->parent_fd = -1;
	}
	if (sess->child_fd != -1)
	{
		close(sess->child_fd);
		sess->child_fd = -1;
	}
}
void priv_sock_set_parent_context(SESSION* sess)
{
	if (sess->child_fd != -1)
	{
		close(sess->child_fd);
		sess->child_fd = -1;
	}
}
void priv_sock_set_child_context(SESSION* sess)
{
	if (sess->parent_fd != -1)
	{
		close(sess->parent_fd);
		sess->parent_fd = -1;
	}
}

void priv_sock_send_cmd(int fd,char cmd)
{
	int ret;
	ret = writen(fd,&cmd,sizeof(cmd));
	if(ret != sizeof(cmd))
	{
		fprintf(stderr,"priv_sock_send_cmd failed\n");
		exit(EXIT_FAILURE);
	}
}
char priv_sock_get_cmd(int fd)
{
	int ret;
	char cmd;
	ret = readn(fd,&cmd,sizeof(cmd));
	if(ret != sizeof(cmd))
	{
		if(ret == 0)
		{
			fprintf(stdout,"client close\n");
			exit(EXIT_SUCCESS);
		}
		else{
			fprintf(stderr,"priv_sock_get_cmd failed\n");
			exit(EXIT_FAILURE);
		}
	}
	return cmd;
}
void priv_sock_send_result(int fd,char res)
{
	int ret;
	ret = writen(fd,&res,sizeof(res));
	if(ret != sizeof(res))
	{
		fprintf(stderr,"priv_sock_send_result failed\n");
		exit(EXIT_FAILURE);
	}
}
char priv_sock_get_result(int fd)
{
	int ret;
	char res;
	ret = readn(fd,&res,sizeof(res));
	if(ret != sizeof(res))
	{
		fprintf(stderr,"priv_sock_get_result failed\n");
		exit(EXIT_FAILURE);
	}
	return res;
}

void priv_sock_send_int(int fd,int the_int)
{
	int ret;
	ret = writen(fd,&the_int,sizeof(the_int));
	if(ret != sizeof(the_int))
	{
		fprintf(stderr,"priv_sock_send_int failed\n");
		exit(EXIT_FAILURE);
	}
}
int priv_sock_get_int(int fd)
{
	int ret;
	int the_int;
	ret = readn(fd,&the_int,sizeof(the_int));
	if(ret != sizeof(the_int))
	{
		fprintf(stderr,"priv_sock_get_int failed\n");
		exit(EXIT_FAILURE);
	}
	return the_int;
}
void priv_sock_send_buf(int fd,const char* buf,unsigned int len)
{
	//字符串是不定长的，因此先发送一个长度
	priv_sock_send_int(fd,(int)len);
	int ret;
	ret = writen(fd,buf,len);
	if(ret != (int)len)
	{
		fprintf(stderr,"priv_sock_send_buf failed\n");
		exit(EXIT_FAILURE);
	}
}
void priv_sock_recv_buf(int fd,char* buf,unsigned int len)
{
	unsigned int checklen = (unsigned int)priv_sock_get_int(fd);
	if(checklen > len)//存不下
	{
		fprintf(stderr,"priv_sock_recv_buf failed1\n");
		exit(EXIT_FAILURE);
	}
	int ret;
	ret = readn(fd,buf,checklen);
	if(ret != (int)checklen)
	{
		fprintf(stderr,"priv_sock_recv_buf failed2\n");
		exit(EXIT_FAILURE);
	}
}
void priv_sock_send_fd(int sock_fd,int fd)
{
	send_fd(sock_fd,fd);
}
int priv_sock_recv_fd(int sock_fd)
{
	return recv_fd(sock_fd);
}

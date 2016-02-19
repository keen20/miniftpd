#include "sysutil.h"

int tcp_client(unsigned short port)
{

	int sock;
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		ERR_EXIT("tcp_client");
	if (port > 0) //表示是主动模式进来的
	{
		int on = 1;
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
			ERR_EXIT("setsockopt");
		struct sockaddr_in localaddr;
		memset(&localaddr, 0, sizeof(localaddr));
		char ip[16] = "192.168.43.131";
		//getlocalip(ip);
		localaddr.sin_family = AF_INET;
		localaddr.sin_port = htons(port);
		localaddr.sin_addr.s_addr = inet_addr(ip);
		if (bind(sock, (struct sockaddr*) &localaddr, sizeof(localaddr)) < 0)
			ERR_EXIT("bind");

	}
	return sock;
}

int tcp_server(const char *host, unsigned short port)
{
	int sock;
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		ERR_EXIT("tcp_server");
	struct sockaddr_in seraddr;
	memset(&seraddr, 0, sizeof(seraddr));
	seraddr.sin_family = PF_INET;
	seraddr.sin_port = htons(port);
	if (host != NULL)
	{
		if (inet_aton(host, &seraddr.sin_addr) == 0) //如果ip地址无效说明是主机名
		{
			struct hostent* hp;
			if ((hp = gethostbyname(host)) == NULL)
				ERR_EXIT("gethostbyname");
			seraddr.sin_addr = *(struct in_addr*) hp->h_addr;
		}
	} else //如果主机名或地址为空，则表示绑定本地主机所有ip
	{
		seraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	int on = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
		ERR_EXIT("setsockopt");

	if (bind(sock, (struct sockaddr*) &seraddr, sizeof(seraddr)) < 0)
		ERR_EXIT("bind");

	if (listen(sock, SOMAXCONN) < 0) //SOMAXCONN表示最大链接数,未完成连接数与已完成连接数之和
		ERR_EXIT("listen");
	//printf("返回时fd是：%d\n",sock);
	return sock;

}

int getlocalip(char *ip)
{
	char host[100] = { 0 };
//获取主机名,存入host
	if (gethostname(host, sizeof(host)) < 0)
		return -1;
//通过主机名获取地址结构
	struct hostent* hp;
	if ((hp = gethostbyname(host)) == NULL)
		return -1;
	strcpy(ip, inet_ntoa(*(struct in_addr*) hp->h_addr));
	return 0;
}
ssize_t readn(int fd, void *buf, size_t count)
{
	size_t nleft = count; //所要读取的字节数
	ssize_t nread;
	char *bufp = (char *) buf; //指向buf缓冲区
	while (nleft > 0) //读取数据，若读取到buf的字节与请求的字节相同，则直接返回请求字节数
	{
		if ((nread = read(fd, bufp, nleft)) < 0) //可能会发生阻塞，要求读取1024，但缓冲区只有10个，在while中阻塞
		{
			if (errno == EINTR)
				continue;
			return -1;
		} else if (nread == 0) //对方关闭或者已经读到eof
			return count - nleft; //返回实际读取的字节数，在此应该小于请求读取的字节数
		bufp += nread;
		nleft -= nread;
	}
	return count;
}
//writen函数包装了write函数，用于写入定长包
ssize_t writen(int fd, const void *buf, size_t count)
{
	size_t nleft = count;
	ssize_t nwritten;
	char *bufp = (char *) buf;

	while (nleft > 0)
	{
		if ((nwritten = write(fd, bufp, nleft)) < 0)
		{
			if (errno == EINTR)
				continue;
			return -1;
		}

		else if (nwritten == 0) //没有写，继续写入数据
			continue;
		bufp += nwritten;
		nleft -= nwritten;
	}
	return count;
}
int read_timeout(int fd, unsigned int wait_seconds)
{
	int ret = 0;
	if (wait_seconds > 0)
	{
		fd_set read_fsets;
		struct timeval timeout;
		FD_ZERO(&read_fsets);
		FD_SET(fd, &read_fsets);
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;
		do
		{
			ret = select(fd + 1, &read_fsets, NULL, NULL, &timeout);
		} while (ret < 0 && errno == EINTR);
		if (ret == 0)
		{
			ret = -1;
			errno = ETIMEDOUT;
		} else if (ret > 0)
		{
			ret = 0;
		}
	}
	return ret;
}
//写超时返回0正常，返回-1错误
int write_timeout(int fd, unsigned int wait_seconds)
{
	int ret = 0;
	if (wait_seconds > 0)
	{
		fd_set write_fsets;
		struct timeval timeout;
		FD_ZERO(&write_fsets);
		FD_SET(fd, &write_fsets);
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;
		do
		{
			ret = select(fd + 1, NULL, &write_fsets, NULL, &timeout);
		} while (ret < 0 && errno == EINTR);
		if (ret == 0)
		{
			ret = -1;
			errno = ETIMEDOUT;
		} else if (ret > 0)
		{
			ret = 0;
		}
	}
	return ret;
}
//接收超时
int accept_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds)
{
	int ret = 0;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	if (wait_seconds > 0)
	{
		fd_set accept_fsets;
		struct timeval timeout;
		FD_ZERO(&accept_fsets);
		FD_SET(fd, &accept_fsets);
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;
		do
		{
			ret = select(fd + 1, &accept_fsets, NULL, NULL, &timeout);
		} while (ret < 0 && errno == EINTR);
		if (ret == -1)
		{
			return -1;
		} else if (ret == 0)
		{
			errno = ETIMEDOUT;
			return -1;
		}
	}
	if (addr != NULL)
		ret = accept(fd, (struct sockaddr*) addr, &addrlen);
	else
		ret = accept(fd, NULL, NULL);
	if (ret == -1)
		ERR_EXIT("accept");
	return ret;
}
void activi_nonblock(int fd)
{
	int ret;
	int flags = fcntl(fd, F_GETFL);
	if (flags == -1)
		ERR_EXIT("fcntl");
	flags |= O_NONBLOCK;
	ret = fcntl(fd, F_SETFL, flags);
	if (ret == -1)
		ERR_EXIT("fcntl");
}
void deactivi_nonblock(int fd)
{
	int ret;
	int flags = fcntl(fd, F_GETFL);
	if (flags == -1)
		ERR_EXIT("fcntl");
	flags &= ~O_NONBLOCK;
	ret = fcntl(fd, F_SETFL, flags);
	if (ret == -1)
		ERR_EXIT("fcntl");
}
int connect_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds)
{
	int ret = 0;
	socklen_t addrlen = sizeof(struct sockaddr_in);
//直接调用connect会阻塞，应先将fd设置为非阻塞模式,在判断超时
	if (wait_seconds > 0)
		activi_nonblock(fd);
	ret = connect(fd, (struct sockaddr*) addr, addrlen);
	if (ret < 0 && errno == EINPROGRESS) //正在处理当中
	{
		fd_set connect_fsets;
		struct timeval timeout;
		FD_ZERO(&connect_fsets);
		FD_SET(fd, &connect_fsets);
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;
		do
		{
			//一旦连接建立，套接字就处于可写状态
			ret = select(fd + 1, NULL, &connect_fsets, NULL, &timeout);
		} while (ret < 0 && errno == EINTR); //被其他信号中断

		if (ret == 0)
		{
			ret = -1;
		} else if (ret > 0)
			ret = 0;
		else
		{/*//ret返回1有两种情况，一种是，连接建立成功并产生了可写事件，一种是套接字产生错误，
		 此时错误不会保存至errno中，需要设置套接字选项*/
			int err;
			socklen_t errlen = sizeof(err);
			int sockopt = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
			if (sockopt == -1)
			{
				return -1;
			}
			if (err == 0)
				ret = 0;
			else
			{
				ret = -1;
				errno = err;
			}
		}
	}
	if (wait_seconds > 0)
		deactivi_nonblock(fd);
	return ret;
}

ssize_t recv_peek(int s, void *buf, size_t len)
{
	while (1)
	{
		int ret = recv(s, buf, len, MSG_PEEK);
		if (ret == -1 && errno == EINTR)
			continue;
		return ret;
	}
}
ssize_t readline(int sockfd, void *buf, size_t maxline)
{
	int ret;
	int nread;
	char *bufp = (char*) buf;
	int nleft = maxline;
	while (1)
	{
		ret = recv_peek(sockfd, bufp, nleft);
		if (ret < 0)
		{
			ERR_EXIT("recv_peek");
		} else if (ret == 0)
		{
			return ret; //客户端退出
		}
		nread = ret;
		int i;
		for (i = 0; i < nread; i++)
		{
			if (bufp[i] == '\n') //有回车表示是完整一行，读取然后返回
			{
				ret = read(sockfd, bufp, i + 1);
				if (ret != i + 1)
					ERR_EXIT("read");
				return ret;
			}
			if (nread > nleft)
				ERR_EXIT("read");
		}
		ret = read(sockfd, bufp, nread); //没回车表示还有剩余数据，读完不饭回
		nleft -= nread;
		bufp += nread;
	}
	return -1; //程序不会走这一步，如果走这一步证明出错了
}

void send_fd(int sockfd, int sendfd) //sendfd，要传递的文件描述字(于辅助数据内)
{
	int ret;
	struct msghdr msg;
	struct cmsghdr* p_cmsg; //辅助数据
	struct iovec vec; //数据
	//根据辅助数据结构体内的数据得到整个结构体大小(包括填充数据)作为缓冲区大小，也可以定义更大缓冲区，传递多个fd
	char cmsgbuf[CMSG_SPACE(sizeof(sendfd))];
	int *p_fds;
	char sendchar = 0;	//不是为了发送正常数据，而是为了发送辅助数据
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);

	p_cmsg = CMSG_FIRSTHDR(&msg);
	p_cmsg->cmsg_len = CMSG_LEN(sizeof(sendfd));
	p_cmsg->cmsg_level = SOL_SOCKET;
	p_cmsg->cmsg_type = SCM_RIGHTS;

	p_fds = (int *) CMSG_DATA(p_cmsg);
	*p_fds = sendfd;
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;	//数据数组中只有一个(只发送一个结构体)
	msg.msg_flags = 0;

	vec.iov_base = &sendchar;	//只发送一个字节
	vec.iov_len = sizeof(sendchar);

	ret = sendmsg(sockfd, &msg, 0);
	if (ret != 1)
		ERR_EXIT("sendmsg");
}

int recv_fd(const int sockfd)
{
	int ret;
	char recvchar;
	int recvfd;
	struct msghdr msg;
	struct cmsghdr* p_cmsg;	//辅助数据
	struct iovec vec;	//数据
	char cmsgbuf[CMSG_SPACE(sizeof(recvchar))];
	int *p_fds;
	vec.iov_base = &recvchar;	//只发送一个字节
	vec.iov_len = sizeof(recvchar);

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;	//数据数组中只有一个(只发送一个结构体)
	msg.msg_flags = 0;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);

	p_cmsg = CMSG_FIRSTHDR(&msg);
	if (p_cmsg == NULL)
		ERR_EXIT("CMSG_FIRSTHDR");
	p_fds = (int *) CMSG_DATA(p_cmsg);
	*p_fds = -1;	//将自己的置空
	ret = recvmsg(sockfd, &msg, 0);
	if (ret != 1)	//因为正常数据发送了一个，它返回发送回接收的字符个数
		ERR_EXIT("recvmsg");

	//重新开始接收一个msg，置于msg本地框架中
	p_cmsg = CMSG_FIRSTHDR(&msg);
	if (p_cmsg == NULL)
		ERR_EXIT("CMSG_FIRSTHDR");
	p_fds = (int *) CMSG_DATA(p_cmsg);
	recvfd = *p_fds;
	return recvfd;
}


const char* statbuf_get_permission(struct stat *f_stat,char* linkbuf,struct dirent * dt)
{
	static char permission[] = "----------";//反之局部变量传递
	permission[0] = '?';
	mode_t mode = f_stat->st_mode;
	switch (mode & S_IFMT)
	{
		case S_IFREG:
			permission[0] = '-';
			break;
		case S_IFSOCK:
			permission[0] = 's';
			break;
		case S_IFLNK:
			permission[0] = 'l';
			int tmp = sprintf(linkbuf, " %s ", "->");
			readlink(dt->d_name, linkbuf + tmp, sizeof(linkbuf));
			break;
		case S_IFBLK:
			permission[0] = 'b';
			break;
		case S_IFDIR:
			permission[0] = 'd';
			break;
		case S_IFCHR:
			permission[0] = 'c';
			break;
		case S_IFIFO:
			permission[0] = 'p';
			break;
	}
	if (mode & S_IRUSR)
	{
		permission[1] = 'r';
	}
	if (mode & S_IWUSR)
	{
		permission[2] = 'w';
	}
	if (mode & S_IXUSR)
	{
		permission[3] = 'x';
	}
	if (mode & S_IRGRP)
	{
		permission[4] = 'r';
	}
	if (mode & S_IWGRP)
	{
		permission[5] = 'w';
	}
	if (mode & S_IXGRP)
	{
		permission[6] = 'x';
	}
	if (mode & S_IROTH)
	{
		permission[7] = 'r';
	}
	if (mode & S_IWOTH)
	{
		permission[8] = 'w';
	}
	if (mode & S_IXOTH)
	{
		permission[9] = 'x';
	}
	if (mode & S_ISUID)
	{
		permission[3] = (permission[3] == 'x') ? 's' : 'S';
	}
	if (mode & S_ISGID)
	{
		permission[6] = (permission[6] == 'x') ? 's' : 'S';
	}
	if (mode & S_ISVTX)
	{
		permission[9] = (permission[9] == 'x') ? 's' : 'S';
	}

	return permission;
}

const char* statbuf_get_date(struct stat *f_stat)
{
	const char* p_date_format = " %b %e %H:%M";
	struct timeval tv;
	gettimeofday(&tv, NULL);
	time_t local_time = tv.tv_sec;
	if (f_stat->st_mtime > local_time
			|| (local_time - f_stat->st_mtime) > 128 * 24 * 60 * 60)
	{
		p_date_format = " %b %e  %Y";
	}
	static char datebuf[64] = { 0 };
	struct tm* p_tm = localtime(&local_time);
	strftime(datebuf, sizeof(datebuf), p_date_format, p_tm);
	return datebuf;
}

int lock_file_read(int fd)
{
	int ret;
	struct flock the_lock;
	the_lock.l_type = F_RDLCK;
	the_lock.l_whence = SEEK_SET;
	the_lock.l_start = 0;
	the_lock.l_len = 0;
	do
	{
	ret = fcntl(fd,F_SETLKW,&the_lock);
	}while(ret < 0 && errno == EINTR);
	return ret;
}
int lock_file_write(int fd)
{
	int ret;
	struct flock the_lock;
	the_lock.l_type =  F_WRLCK;
	the_lock.l_whence = SEEK_SET;
	the_lock.l_start = 0;
	the_lock.l_len = 0;
	do
	{
	ret = fcntl(fd,F_SETLKW,&the_lock);
	}while(ret < 0 && errno == EINTR);
	return ret;
}
int unlock_file(int fd)
{
	int ret;
	struct flock the_lock;
	the_lock.l_type =  F_UNLCK;
	the_lock.l_whence = SEEK_SET;
	the_lock.l_start = 0;
	the_lock.l_len = 0;
	do
	{
	ret = fcntl(fd,F_SETLK,&the_lock);
	}while(ret < 0 && errno == EINTR);
	return ret;
}
static struct timeval curr_time;
long get_time_sec(void)
{
	if(gettimeofday(&curr_time,NULL) < 0)
	{
		ERR_EXIT("gettimeofday");
	}
	return curr_time.tv_sec;
}
long get_time_usec(void)
{
	//两个调用就会不准确，因此做一个静态变量
	return curr_time.tv_usec;
}
void highlevel_sleep(double seconds)
{
	time_t sec = (time_t) seconds;//秒
	double xiaoshu = seconds-(double)sec;//小数部分
	struct timespec ts;
	ts.tv_sec = sec;
	ts.tv_nsec = (long)(xiaoshu * (double)1000000000);

	int ret;
	do{
		ret = nanosleep(&ts,&ts);
	}while(ret == -1 && errno == EINTR);

}

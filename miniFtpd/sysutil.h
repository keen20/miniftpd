#ifndef _SYSUTIL_H_
#define _SYSUTIL_H_
#include "common.h"//将函数用到的头文件整合到common头文件中
#include "session.h"
/*
 * tcp_server：启动tcp服务器
 * host与port：主机名称，与端口
 * 成功返回监听套接字
 */
int tcp_server(const char *host,unsigned short port);
int tcp_client(unsigned short port);//服务器绑定端口
int getlocalip(char *ip);
ssize_t readn(int fd, void *buf, size_t count);
ssize_t writen(int fd, const void *buf, size_t count);
int read_timeout(int fd, unsigned int wait_seconds);
int write_timeout(int fd, unsigned int wait_seconds);
int accept_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds);
void activi_nonblock(int fd);
void deactivi_nonblock(int fd);
int connect_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds);
ssize_t readline(int sockfd, void *buf, size_t maxline);
ssize_t recv_peek(int s, void *buf, size_t len);
void send_fd(int sockfd,int sendfd);
int recv_fd(const int sockfd);
//获取文件权限
const char* statbuf_get_permission(struct stat *f_stat,char* linkbuf,struct dirent * dt);
//获取日期
const char* statbuf_get_date(struct stat *f_stat);
//读写锁
int lock_file_read(int fd);
int lock_file_write(int fd);
int unlock_file(int fd);
long get_time_sec();
long get_time_usec();
void highlevel_sleep(double seconds);

#endif

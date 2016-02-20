#include "ftpproto.h"
#include "qstring.h"
#include "ftpcode.h"
#include "turnable.h"
#include "privatesock.h"
void ftp_reply(SESSION* sess, int status, const char* text); //构造回复消息，并发送
void ftp_notspacedivide_reply(SESSION* sess, int status, const char* text); //以-分割的消息
int list_common(SESSION* sess,int judje);//判断是否只传送文件名
void rate_limit(SESSION* sess,int transfer_bytes,int is_upload);
void upload_common(SESSION* sess,int flag);
int get_transfer_fd(SESSION* sess);
static void do_user(SESSION* sess); //static 表示只用于当前模块
static void do_pass(SESSION* sess);
static void do_syst(SESSION* sess);
//static void do_feat(SESSION* sess);
static void do_type(SESSION* sess);
static void do_stru(SESSION* sess);
static void do_mode(SESSION* sess);
static void do_pwd(SESSION* sess);
static void do_cwd(SESSION* sess);
static void do_cdup(SESSION* sess);
static void do_mkd(SESSION* sess);
static void do_rmd(SESSION* sess);
static void do_dele(SESSION* sess);
static void do_rest(SESSION* sess);
static void do_size(SESSION* sess);
static void do_rnfr(SESSION* sess);
static void do_rnto(SESSION* sess);
static void do_retr(SESSION* sess);
static void do_stor(SESSION* sess);
static void do_appe(SESSION* sess);
static void do_noop(SESSION* sess);
static void do_quit(SESSION* sess);
static void do_help(SESSION* sess);
static void do_stat(SESSION* sess);
static void do_site(SESSION* sess);
static void do_list(SESSION* sess);
static void do_nlst(SESSION* sess);
static void do_pasv(SESSION* sess);
static void do_port(SESSION* sess);
static void do_abor(SESSION* sess);
int port_active(SESSION* sess);
int pasv_active(SESSION* sess);
typedef struct ftpcmd
{
	const char* cmd;
	void (*cmd_handler)(SESSION* sess);
} ftpcmd_t;

static ftpcmd_t crtl_cmds[] = {
/*访问控制命令*/
{ "USER", do_user }, { "PASS", do_pass }, { "CWD", do_cwd }, { "XCWD", do_cwd },
		{ "CDUP", do_cdup }, { "XCUP", do_cdup }, { "QUIT", do_quit }, { "ACCT",
				NULL }, { "SMNT", NULL }, { "REIN", NULL },
		/*传输参数命令*/
		{ "PASV", do_pasv }, { "PORT", do_port }, { "TYPE", do_type }, { "STRU",
				do_stru }, { "MODE", do_mode },
		/*服务命令*/
		{ "RETR", do_retr }, { "STOR", do_stor }, { "APPE", do_appe }, { "LIST",
				do_list }, { "NLST", do_nlst }, { "REST", do_rest }, { "ABOR",
				do_abor }, { "\377\364\377\362ABOR", do_abor },
		{ "PWD", do_pwd }, { "XPWD", do_pwd }, { "MKD", do_mkd }, { "XMKD",
				do_mkd }, { "RMD", do_rmd }, { "XRMD", do_rmd }, { "DELE",
				do_dele }, { "RNFR", do_rnfr }, { "RNTO", do_rnto }, { "SITE",
				do_site }, { "SYST", do_syst }, { "FEAT",/*do_feat*/NULL }, {
				"SIZE", do_size }, { "STAT", do_stat }, { "NOOP", do_noop }, {
				"HELP", do_help }, { "STOP", NULL }, { "ALLO", NULL } };

void handle_child(SESSION* sess)
{
	int ret;
	ftp_reply(sess, FTP_GREET, "(miniftpd 0.1)");
	while (1)
	{
		memset(sess->cmdline, 0, MAX_COMMAND_LINE);
		memset(sess->cmd, 0, sizeof(sess->cmd));
		memset(sess->args, 0, sizeof(sess->args));
		ret = readline(sess->conn, sess->cmdline, MAX_COMMAND_LINE);
		if (ret < 0)
		{
			ERR_EXIT("realine");
			//关闭nobody进程(未实现)
		} else if (ret == 0) //客户端断开
		{
			exit(EXIT_SUCCESS);
			//关闭nobody进程(未实现)
		}
		/*解析FTP命令与参数 USER xxxx\r\n 其中\r\n是ftp协议要求的*/

		//1,去除\r\n
		str_trim_crlf(sess->cmdline);
		//printf("[%s]\n",sess->cmdline);
		//2,解析命令，分割
		str_split(sess->cmdline, sess->cmd, sess->args, ' ');
		//printf("cmd=[%s],args=[%s]\n",sess->cmd,sess->args);
		//3,将命令大写
		str_to_upper(sess->cmd);
		//处理命令
		int i;
		int size = sizeof(crtl_cmds) / sizeof(crtl_cmds[0]);
		for (i = 0; i < size; i++)
		{
			if (strcmp(crtl_cmds[i].cmd, sess->cmd) == 0)
			{
				if (crtl_cmds[i].cmd_handler != NULL)
					crtl_cmds[i].cmd_handler(sess);
				else
					ftp_reply(sess, FTP_COMMANDNOTIMPL, "Unimplement command");
				break;
			}
		}
		if (i == size)
		{
			ftp_reply(sess, FTP_BADCMD, "Unknown command");
		}
	}
}

static void do_user(SESSION* sess)
{
	struct passwd* pw = getpwnam(sess->args);		//从系统查找用户
	if (pw == NULL)
	{
		ftp_reply(sess, FTP_LOGINERR, "Login incorrect");
		return;
	}
	sess->uid = pw->pw_uid;
	ftp_reply(sess, FTP_GIVEPWORD, "Please specify the password");
}
static void do_pass(SESSION* sess)
{
	//二次根据uid验证
	struct passwd* pw = getpwuid(sess->uid);
	if (pw == NULL)
	{
		ftp_reply(sess, FTP_LOGINERR, "Login incorrect");
		return;
	}

	//从影子文件获取密码
	struct spwd* sp = getspnam(pw->pw_name);
	if (sp == NULL)
	{
		ftp_reply(sess, FTP_LOGINERR, "Login incorrect");
		return;
	}
	//将明文密码加密，并以影子文件获取的密码比较
	char *encrypted_pass = crypt(sess->args, sp->sp_pwdp);		//种子是已加密过的密码
	if (strcmp(encrypted_pass, sp->sp_pwdp) != 0)
	{
		ftp_reply(sess, FTP_LOGINERR, "Login incorrect");
		return;
	}
	//登陆成功之后将root权限改为登陆用户的权限
	umask(turnable_local_umask);
	setegid(pw->pw_gid);
	seteuid(pw->pw_uid);
	//工作目录改为该用户家目录
	chdir(pw->pw_dir);
	ftp_reply(sess, FTP_LOGINOK, "Login successfully");
}

static void do_syst(SESSION* sess)
{
	ftp_reply(sess, FTP_SYSTOK, "UNIX Type: L8");
}
/*static void do_feat(SESSION* sess)
 {
 ftp_notspacedivide_reply(sess,FTP_FEAT,"Features:");
 writen(sess->conn," EPRT\r\n",strlen(" EPRT\r\n"));
 writen(sess->conn," EPSV\r\n",strlen(" EPSV\r\n"));
 writen(sess->conn," MDTM\r\n",strlen(" MDTM\r\n"));
 writen(sess->conn," PASV\r\n",strlen(" PASV\r\n"));
 writen(sess->conn," EPRT\r\n",strlen(" EPRT\r\n"));
 writen(sess->conn," REST STREAM\r\n",strlen(" REST STREAM\r\n"));
 writen(sess->conn," SIZE\r\n",strlen(" SIZE\r\n"));
 writen(sess->conn," TVFS\r\n",strlen(" TVFS\r\n"));
 writen(sess->conn," UTF8\r\n",strlen(" UTF8\r\n"));
 ftp_notspacedivide_reply(sess,FTP_FEAT,"End");
 printf("in do_feat!!!\n");
 }*/
static void do_type(SESSION* sess)
{
	if (strcmp(sess->args, "A") == 0)
	{
		sess->is_ascii = 1;
		ftp_reply(sess, FTP_TYPEOK, "Switching to ASCII mode");
	} else if (strcmp(sess->args, "I") == 0)
	{
		sess->is_ascii = 0;
		ftp_reply(sess, FTP_TYPEOK, "Switching to Binary mode");
	} else
	{
		ftp_reply(sess, FTP_BADCMD, "Unrecognised Type command");
	}
}
static void do_stru(SESSION* sess)
{
	printf("int the do_pwd1\n");
}
static void do_mode(SESSION* sess)
{
	printf("int the do_pwd1\n");
}
static void do_pwd(SESSION* sess)		//获取当前路径
{
	char text[1024] = { 0 };
	char dir[1024] = { 0 };
	getcwd(dir, sizeof(dir));
	sprintf(text, "\"%s\"", dir);
	ftp_reply(sess, FTP_PWDOK, text);
}
static void do_cwd(SESSION* sess)
{
	if(chdir(sess->args) < 0)
	{
	ftp_reply(sess,FTP_FILEFAIL,"Failed to change directory");
		return;
	}
	ftp_reply(sess,FTP_CWDOK,"Directory successfully changed");
}
static void do_cdup(SESSION* sess)
{
	if(chdir("..") < 0)
	{
	ftp_reply(sess,FTP_FILEFAIL,"Failed to change directory");
		return;
	}
	ftp_reply(sess,FTP_CWDOK,"Directory successfully changed");
}
static void do_mkd(SESSION* sess)
{
	if(mkdir(sess->args,0777) < 0)
	{
		ftp_reply(sess,FTP_FILEFAIL,"Create directory operation failed");
		return;
	}
	char text[1024+1]={0};
	if(sess->args[0] == '/')//跟路径
	{
	sprintf(text,"%s created",sess->args);
	}
	else
	{
		//获取当前路径
		char dir[1024+1] = {0};
		getcwd(dir,1024);
		if(dir[strlen(dir)-1] == '/')
		{
			sprintf(text,"%s%s created",dir,sess->args);
		}
	}
	ftp_reply(sess,FTP_MKDIROK,text);
}
static void do_rmd(SESSION* sess)
{
	if(rmdir(sess->args) < 0)
	{
		ftp_reply(sess,FTP_FILEFAIL,"Remove directory operation failed");
		return;
	}
	else
	{
		ftp_reply(sess,FTP_RMDIROK,"Remove directory operation successfully");
	}
}
static void do_dele(SESSION* sess)
{
	if(unlink(sess->args) < 0)
	{
		ftp_reply(sess,FTP_FILEFAIL,"Delete operation failed");
		return;
	}
	else
	{
		ftp_reply(sess,FTP_DELEOK,"Delete operation successfully");
	}
}
static void do_rest(SESSION* sess)
{
	long long restart_pos = str_to_longlong(sess->args);
	char text[1024] = {0};
	sprintf(text,"Restart position accepted (%lld)",restart_pos);
	ftp_reply(sess,FTP_RESTOK,text);
}
static void do_size(SESSION* sess)
{
	struct stat fstat;
	if(stat(sess->args,&fstat) < 0)
	{
		ftp_reply(sess,FTP_FILEFAIL,"SIZE operation failed");
		return;
	}
	if(!S_ISREG(fstat.st_mode))
	{
		ftp_reply(sess,FTP_FILEFAIL,"Could not get file size");
		return;
	}
	char text[1024] = {0};
	sprintf(text,"%lld",(long long)fstat.st_size);
	ftp_reply(sess,FTP_SIZEOK,text);
}
static void do_rnfr(SESSION* sess)
{
	sess->rnfr_mark = (char*)malloc(strlen(sess->args)+1);
	memset(sess->rnfr_mark,0,strlen(sess->args)+1);
	strcpy(sess->rnfr_mark,sess->args);
	ftp_reply(sess,FTP_RNFROK,"Ready for RNTR");
}
static void do_rnto(SESSION* sess)
{
	if(sess->rnfr_mark == NULL)
	{
		ftp_reply(sess,FTP_NEEDRNFR,"FNFR required first");
		return;
	}
		rename(sess->rnfr_mark,sess->args);
		ftp_reply(sess,FTP_RMNAMEOK,"Rename successfully");
		free(sess->rnfr_mark);
		sess->rnfr_mark = NULL;
}
static void do_retr(SESSION* sess)
{
	//下载文件
	//断点续传
	if (get_transfer_fd(sess) == 0)
		return;
	//读取断点
	long long offset = sess->restart_position;
	sess->restart_position = 0;
	//打开文件
	int fd = open(sess->args,O_RDONLY);
	if(fd < 0)
	{
	ftp_reply(sess, FTP_FILEFAIL, "Fail to open file");
		return;
	}
	//加读锁
	int ret;
	ret = lock_file_read(fd);
	if(ret < 0)
	{
	ftp_reply(sess, FTP_FILEFAIL, "Fail to open file");
		return;
	}
	//判断是否是普通文件，设备文件是无法传输的
	struct stat f_stat;
	ret = fstat(fd,&f_stat);
	if(!S_ISREG(f_stat.st_mode))
	{
	ftp_reply(sess, FTP_FILEFAIL, "Fail to open file");
		return;
	}

	if(offset != 0)
	{
		ret = lseek(fd,offset,SEEK_SET);
		if(ret == -1)
		{
			ftp_reply(sess, FTP_FILEFAIL, "Fail to open file");
			return;
		}

	}

	//150
	char text[1024] = {0};
	if(sess->is_ascii)
	{
		sprintf(text,"Opening ASCII mode data connection for %s (%lld bytes)"
				,sess->args,(long long)f_stat.st_size);
	}
	else
	{
		sprintf(text,"Opening BINARY mode data connection for %s (%lld bytes)"
				,sess->args,(long long)f_stat.st_size);
	}
	ftp_reply(sess, FTP_DATACONN, text);
	//下载文件
	int flag = 0;
	/*char buf[4096] = {0};
	int flag = 0;//标记是哪种退出
	while(1)
	{
		ret = read(fd,buf,sizeof(buf));
		if(ret == -1)
		{
			if(errno == EINTR)
			{
				continue;
			}
			else
			{
				flag = 1;
				break;
			}
		}
		else if(ret == 0)
		{
			break;
		}

		if(writen(sess->data_fd,buf,ret) != ret)
		{
			flag = 2;
			break;
		}

	}*/

	long long bytes_to_send = f_stat.st_size;
	if(offset > bytes_to_send)
	{
		bytes_to_send = 0;
	}
	else
	{
		bytes_to_send -= offset;
	}
	sess->bw_transfer_start_sec = get_time_sec();
	sess->bw_transfer_start_usec = get_time_usec();
	while(bytes_to_send)
	{
		int num_this_time = bytes_to_send > 4096 ? 4096 : bytes_to_send;
		ret = sendfile(sess->data_fd,fd,NULL,num_this_time);
		if(ret == -1)
		{
			flag = 2;
			break;
		}
		rate_limit(sess,ret,0);
		bytes_to_send -= ret;
	}

	if(bytes_to_send == 0)
		flag = 0;

	if(flag == 0)
	{
		ftp_reply(sess, FTP_TRANSFEROK, "Transfer complete");
	}
	else if(flag == 1)
	{
		ftp_reply(sess, FTP_BADSENDFILE, "Failure reading from local file");
	}
	else if(flag == 2)
	{
		ftp_reply(sess, FTP_BADSENDNET, "Failure writting to network stream");
	}

	unlock_file(fd);
	//关闭传输套接字
	close(sess->data_fd);
	sess->data_fd = -1;
	close(fd);
	//226
	ftp_reply(sess, FTP_TRANSFEROK, "Directory send OK");

}
static void do_stor(SESSION* sess)
{
	upload_common(sess,0);
}
static void do_appe(SESSION* sess)
{
	upload_common(sess,1);
}
static void do_noop(SESSION* sess)
{
	printf("int the do_pwd1\n");
}
static void do_quit(SESSION* sess)
{
	printf("int the do_pwd1\n");
}
static void do_help(SESSION* sess)
{
	printf("int the do_pwd1\n");
}
static void do_stat(SESSION* sess)
{
	printf("int the do_pwd1\n");
}
static void do_site(SESSION* sess)
{
	printf("int the do_pwd1\n");
}
static void do_list(SESSION* sess)
{
	//创建数据连接
	if (get_transfer_fd(sess) == 0)
		return;
	//150
	ftp_reply(sess, FTP_DATACONN, "Here comes directory listing");
	//传输列表
	list_common(sess,1);//完全传送
	//关闭传输套接字
	close(sess->data_fd);
	sess->data_fd = -1;
	//226
	ftp_reply(sess, FTP_TRANSFEROK, "Directory send OK");
}
static void do_nlst(SESSION* sess)
{
	list_common(sess,0);//只传送文件名
}
static void do_pasv(SESSION* sess)
{
	//服务器创建套接字,并指定端口
	char ip[16] = PASV_MODE_SERVER_IP;
	//char ip[16] = {0};
	//getlocalip(ip);

	/*sess->pasv_listen_fd = tcp_server(ip, 0);	//0表示主机动态分配端口
	struct sockaddr_in seraddr;
	memset(&seraddr, 0, sizeof(seradqdr));
	socklen_t seraddrlen = sizeof(seraddr);
	if (getsockname(sess->pasv_listen_fd, (struct sockaddr *) &seraddr, &seraddrlen) < 0)
	{
		ERR_EXIT("getsockname");
	}
	unsigned short port = ntohs(seraddr.sin_port);*/

	priv_sock_send_cmd(sess->child_fd,PRIV_SOCK_PASV_LISTEN);//获取被动模式监听端口
	unsigned short port = (int)priv_sock_get_int(sess->child_fd);
	//sess->pasv_listen_fd = priv_sock_get_int(sess->child_fd);
	unsigned int v[4];
	sscanf(ip, "%u.%u.%u.%u", &v[0], &v[1], &v[2], &v[3]);
	char buf[1024] = { 0 };
	//unsigned char* p = (unsigned char*) &port;
	sprintf(buf, "Entering Passive Mode (%u,%u,%u,%u,%u,%u)", v[0], v[1], v[2],
			v[3], port>>8,port&0xFF);
	ftp_reply(sess, FTP_PASVOK, buf);
}
static void do_port(SESSION* sess)
{
	//PORT 192,168,1,100,233,122其中233是端口的高八位，122是低八位
	unsigned int v[6] = { 0 };
	//从sess->args获取数据按照一定的格式格式化到v[6]中
	sscanf(sess->args, "%u,%u,%u,%u,%u,%u,%u", &(v[2]), &(v[3]), &(v[4]), &(v[5]),
			&(v[0]), &(v[1]));
	//记住要释放
	sess->port_addr = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
	memset(sess->port_addr, 0, sizeof(struct sockaddr_in));
	sess->port_addr->sin_family = AF_INET;
	unsigned char* p = (unsigned char*) &sess->port_addr->sin_port;
	p[0] = v[0];
	p[1] = v[1];
	//sess->port_addr->sin_port = htons(sess->port_addr->sin_port);

	p = (unsigned char*) &sess->port_addr->sin_addr;
	p[0] = v[2];
	p[1] = v[3];
	p[2] = v[4];
	p[3] = v[5];

	ftp_reply(sess, FTP_PORTOK,
			"PORT command successfully. Consider using PASV");

	//printf("int the do_port ip=%s\n", inet_ntoa((sess->port_addr->sin_addr)));

}
static void do_abor(SESSION* sess)
{
	printf("int the do_pwd1\n");
}

void ftp_reply(SESSION* sess, int status, const char* text)
{
	char buf[1024] = { 0 };
	sprintf(buf, "%d %s.\r\n", status, text);
	writen(sess->conn, buf, strlen(buf));
}
void ftp_notspacedivide_reply(SESSION* sess, int status, const char* text)
{
	char buf[1024] = { 0 };
	sprintf(buf, "%d-%s\r\n", status, text);
	writen(sess->conn, buf, strlen(buf));
}
int list_common(SESSION* sess,int judje)//判断是否只传送文件名
{
	DIR *dir = opendir(".");	//打开当前目录
	if (dir == NULL)
		return 0;
	//遍历目录中文件
	struct dirent * dt;
	struct stat f_stat;
	while ((dt = readdir(dir)) != NULL)
	{
		char linkbuf[1024] = { 0 };	//用于处理软连接
		if (lstat(dt->d_name, &f_stat) < 0)
			continue;
		if (dt->d_name[0] == '.')
			continue;
		char buf[1024] = { 0 };
		if(judje)
		{
		int off = 0;	//一定要初始化
		const char* permission = statbuf_get_permission(&f_stat,linkbuf,dt);
		off += sprintf(buf, "%s ", permission);
		off += sprintf(buf + off, "% 3d %-8d %-8d ", (int) f_stat.st_nlink,
				(int) f_stat.st_uid, (int) f_stat.st_gid);
		off += sprintf(buf + off, "%8lu ", (unsigned long) f_stat.st_size);
		//获取日期
		const char* datebuf = statbuf_get_date(&f_stat);

		off += sprintf(buf + off, "%s ", datebuf);
		sprintf(buf + off, "%s%s\r\n", dt->d_name, linkbuf);
		}
		else
		{
			sprintf(buf, "%s\r\n", dt->d_name);
		}
		writen(sess->data_fd, buf, strlen(buf));
	}
	closedir(dir);
	return 1;
}

int port_active(SESSION* sess)
{
	if (sess->port_addr)
	{
		if (pasv_active(sess))
		{
			fprintf(stderr, "pasv mode is already active");	//人工提示语句用fprintf，系统可提示用perror与errno
			exit(EXIT_FAILURE);
		}
		return 1;
	}
	return 0;
}
int pasv_active(SESSION* sess)
{
	/*if (sess->pasv_listen_fd != -1)//监听套接字是由nobody进程创建，因此服务进程的监听套接字始终是-1
	{
		if (port_active(sess))
		{
			fprintf(stderr, "port mode is already active");	//人工提示语句用fprintf，系统可提示用perror与errno
			exit(EXIT_FAILURE);
		}
		return 1;
	}*/
	priv_sock_send_cmd(sess->child_fd,PRIV_SOCK_PASV_ACTIVE);
	int active = priv_sock_get_int(sess->child_fd);
	if(active)
	{
		if (port_active(sess))
		{
			fprintf(stderr, "port mode is already active");	//人工提示语句用fprintf，系统可提示用perror与errno
			exit(EXIT_FAILURE);
		}
		return 1;
	}
	return 0;
}
int get_portmode_fd(SESSION* sess)
{
	/*
	 * 向nobody进程发送PRIV_SOCK_GET_DATA_SOCK命令
	 * 向nobody进程发送port
	 * 向nobody进程发送ip
	 */
	priv_sock_send_cmd(sess->child_fd,PRIV_SOCK_GET_DATA_SOCK);
	unsigned short port = ntohs(sess->port_addr->sin_port);
	char* ip = inet_ntoa(sess->port_addr->sin_addr);
	priv_sock_send_int(sess->child_fd,(int)port);
	priv_sock_send_buf(sess->child_fd,ip,strlen(ip));

	//接收nobody返回的应答
	char res = priv_sock_get_result(sess->child_fd);
	if(res == PRIV_SOCK_RESULT_BAD)
	{
		return 0;
	}
	else if(res == PRIV_SOCK_RESULT_OK)
	{
		sess->data_fd = priv_sock_recv_fd(sess->child_fd);
	}
	return 1;
}

int get_pasvmode_fd(SESSION* sess)
{
	priv_sock_send_cmd(sess->child_fd,PRIV_SOCK_PASV_ACCEPT);
	char res = priv_sock_get_result(sess->child_fd);
	if(res == PRIV_SOCK_RESULT_OK)
	{
		sess->data_fd = priv_sock_recv_fd(sess->child_fd);
	}
	else if(res == PRIV_SOCK_RESULT_BAD)
	{
		return 0;
	}
	return 1;
}

int get_transfer_fd(SESSION* sess)
{
	//printf("port_addr不为空是：%d\n",port_active(sess));
	//printf("get_transfer_fd的sess->pasv_listen_fd=%d\n",sess->pasv_listen_fd);
	if (!port_active(sess) && !pasv_active(sess))
	{
		//必须给客户端应答，不然客户端会等待阻塞（例如单独使用LIST原始命令而未经过port或pasv就会阻塞）
		ftp_reply(sess, FTP_BADSENDCONN, "Use PORT or PASV first");
		return 0;
	}
	//如果是主动模式
	int ret = 1;
	if (port_active(sess))
	{
		/*创建socket
		 * bind 20
		 * connect
		 */
		/*int fd = tcp_client(0);
		if (connect_timeout(fd, sess->port_addr, turnable_connect_timeout) < 0)
		{
			close(fd);
			return 0;
		}
		//保存数据套接字
		sess->data_fd = fd;*/
		if(get_portmode_fd(sess) == 0)
		{
			ret = 0;
		}
	}
	//如果是被动模式
	if(pasv_active(sess))
	{
		/*//创建数据套接字失败
		int fd;
		if((fd = accept_timeout(sess->pasv_listen_fd,NULL,turnable_accept_timeout)) < 0)
		{
			return 0;
		}
		//关闭监听套接字，以后接到命令重新监听连接
		close(sess->pasv_listen_fd);
		//创建成功
		sess->data_fd = fd;*/
		if(get_pasvmode_fd(sess) == 0)
		{
			ret = 0;
		}
	}

	//释放资源
	if (sess->port_addr)
	{
		free(sess->port_addr);
		sess->port_addr = NULL;
	}
	return ret;
	//被动模式
}

//判断是否需要限速,并限速
void rate_limit(SESSION* sess,int transfer_bytes,int is_upload)
{
   long curr_sec = get_time_sec();
   long curr_usec = get_time_usec();
	//计算传输bytes所用的时间

	double time_flows;
	time_flows = curr_sec - sess->bw_transfer_start_sec;
	time_flows += (double)(curr_usec - sess->bw_transfer_start_usec)/(double)1000000;

	if(time_flows <= 0)
	{
		time_flows = 0.01;
	}
	//计算当前传输速度
	unsigned int curr_trans_rate = (unsigned int)((double)transfer_bytes / time_flows);

	//睡眠时间 = (当前传输速度/最大传输速度-1)*当前传输时间
	//计算比率(当前传输速度/最大传输速度)
	double rate_raio;
	if(is_upload)
	{
		if(curr_trans_rate <= sess->bw_upload_rate_max)
		{
			return;
		}
		rate_raio = curr_trans_rate / sess->bw_upload_rate_max;
	}
	else
	{
		if(curr_trans_rate <= sess->bw_download_rate_max)
		{
			return;
		}
		rate_raio = curr_trans_rate / sess->bw_download_rate_max;
	}
	//计算睡眠时间
	double pause_time;
	pause_time = (rate_raio - (double)1) * time_flows;
	highlevel_sleep(pause_time);

	//更改下一次传输时间
	sess->bw_transfer_start_sec = get_time_sec();
	sess->bw_transfer_start_usec = get_time_usec();
}
void upload_common(SESSION* sess,int is_append)
{
	if (get_transfer_fd(sess) == 0)
			return;
		//读取断点
		long long offset = sess->restart_position;
		sess->restart_position = 0;
		//打开文件
		int fd = open(sess->args,O_CREAT|O_WRONLY,0777);
		if(fd < 0)
		{
		ftp_reply(sess, FTP_UPLOADFAIL, "Could not create file1");
			return;
		}
		//加读锁
		int ret;
		ret = lock_file_write(fd);
		if(ret < 0)
		{
		ftp_reply(sess, FTP_UPLOADFAIL, "Could not create file2");
			return;
		}
		struct stat f_stat;
		ret = fstat(fd,&f_stat);
		//不需要判断是否是普通文件因为是上传
		/*struct stat f_stat;
		ret = fstat(fd,&f_stat);
		if(!S_ISREG(f_stat.st_mode))
		{
		ftp_reply(sess, FTP_FILEFAIL, "Fail to open file");
			return;
		}*/

		//PTOR
		//REST +　PTOR
		//APPE
		if(!is_append)
		{
			if(offset == 0)
			{
				ftruncate(fd,0);
				if(lseek(fd,offset,SEEK_SET) < 0)
				{
					ftp_reply(sess, FTP_UPLOADFAIL, "Could not create file3");
						return;
				}
			}
			else
			{
				if(lseek(fd,offset,SEEK_SET) < 0)
				{
					ftp_reply(sess, FTP_UPLOADFAIL, "Could not create file4");
						return;
				}
			}
		}


		//150
		char text[1024] = {0};
		if(sess->is_ascii)
		{
			sprintf(text,"Opening ASCII mode data connection for %s (%lld bytes)"
					,sess->args,(long long)f_stat.st_size);
		}
		else
		{
			sprintf(text,"Opening BINARY mode data connection for %s (%lld bytes)"
					,sess->args,(long long)f_stat.st_size);
		}
		ftp_reply(sess, FTP_DATACONN, text);
		//上传文件
		//睡眠时间 = (当前传输速度/最大传输速度-1)*当前传输时间
		sess->bw_transfer_start_sec = get_time_sec();
		sess->bw_transfer_start_usec = get_time_usec();
		char buf[1024] = {0};
		int flag = 0;//标记是哪种退出
		while(1)
		{
			ret = read(sess->data_fd,buf,sizeof(buf));
			if(ret == -1)
			{
				if(errno == EINTR)
				{
					continue;
				}
				else
				{
					flag = 1;
					break;
				}
			}
			else if(ret == 0)
			{
				break;
			}

			//判断是否需要限速,并限速
			rate_limit(sess,ret,1);

			if(writen(fd,buf,ret) != ret)
			{
				flag = 2;
				break;
			}

		}

		if(flag == 0)
		{
			ftp_reply(sess, FTP_TRANSFEROK, "Transfer complete");
		}
		else if(flag == 1)
		{
			ftp_reply(sess, FTP_BADSENDNET, "Failure reading from network stream");
		}
		else if(flag == 2)
		{
			ftp_reply(sess, FTP_BADSENDFILE, "Failure writting to local file");
		}

		unlock_file(fd);
		//关闭传输套接字
		close(sess->data_fd);
		sess->data_fd = -1;
		close(fd);
		//226
		ftp_reply(sess, FTP_TRANSFEROK, "Directory send OK");
}

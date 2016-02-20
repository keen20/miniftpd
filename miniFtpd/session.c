#include "session.h"
#include "ftpproto.h"
#include "privateparent.h"
#include "privatesock.h"
void begin_session(SESSION* sess)
{
	//由于父子间要通讯，因此要创建socketpair
	priv_sock_init(sess);
	pid_t pid;
	pid = fork();
	if (pid < 0)
		ERR_EXIT("fork");
	else if (pid == 0) //子进程服务进程
	{
		priv_sock_set_child_context(sess);
		handle_child(sess);

	} else //父进程nobody进程
	{
		priv_sock_set_parent_context(sess);
		handle_parent(sess);
	}

}

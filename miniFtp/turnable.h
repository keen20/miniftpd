#ifndef _TURNABLE_H_
#define _TURNABLE_H_
#include "common.h"

//初始化默认值
int turnable_pasv_enable;
int turnable_port_enable;
unsigned int turnable_listen_port;
unsigned int turnable_max_clients;
unsigned int turnable_max_per_ip;
unsigned int turnable_accept_timeout;
unsigned int turnable_connect_timeout;
unsigned int turnable_idle_session_timeout;
unsigned int turnable_data_connection_timeout;
unsigned int turnable_local_umask;
unsigned int turnable_upload_max_rate;
unsigned int turnable_download_max_rate;
const char* turnable_listen_address;

#endif

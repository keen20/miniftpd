#include "turnable.h"

//初始化默认值
int turnable_pasv_enable = 1;
int turnable_port_enable = 1;
unsigned int turnable_listen_port = 21;
unsigned int turnable_max_clients = 2000;
unsigned int turnable_max_per_ip = 50;
unsigned int turnable_accept_timeout = 60;
unsigned int turnable_connect_timeout = 60;
unsigned int turnable_idle_session_timeout = 300;
unsigned int turnable_data_connection_timeout = 900;
unsigned int turnable_local_umask = 022;
unsigned int turnable_upload_max_rate = 0;
unsigned int turnable_download_max_rate = 0;
const char* turnable_listen_address;




#include "parseconf.h"
#include "turnable.h"
#include "qstring.h"
#include "parseconf.h"
static struct parseconf_bool_setting
{
		const char* p_setting_name;
		int* p_variable;
}parseconf_bool_array[]=
{
		{"pasv_enable",&turnable_pasv_enable},
		{"port_enable",&turnable_port_enable},
		{NULL,NULL}
};
static struct parseconf_uint_setting
{
		const char* p_setting_name;
		unsigned int* p_variable;
}parseconf_uint_array[]=
{
		{"listen_port",&turnable_listen_port},
		{"max_clients",&turnable_max_clients},
		{"max_per_ip",&turnable_max_per_ip},
		{"accept_timeout",&turnable_accept_timeout},
		{"connect_timeout",&turnable_connect_timeout},
		{"idle_session_timeout",&turnable_idle_session_timeout},
		{"data_connection_timeout",&turnable_data_connection_timeout},
		{"local_umask",&turnable_local_umask},
		{"upload_max_rate",&turnable_upload_max_rate},
		{"download_max_rate",&turnable_download_max_rate},
		{NULL,NULL}
};
static struct parseconf_str_setting
{
		const char* p_setting_name;
		const char** p_variable;
}parseconf_str_array[]=
{
		{"listen_address",&turnable_listen_address},
		{NULL,NULL}
};
void parseconf_load_file(const char* path)
{
	FILE* fp = fopen(path,"r");
	if(fp == NULL)
		ERR_EXIT("fopen");
	char setting_line[1024] = {0};
	while(fgets(setting_line,sizeof(setting_line),fp) != NULL)
	{
		if(strlen(setting_line) == 0
			|| setting_line[0] == '#'
			|| str_all_space(setting_line))
			continue;
		//fgets读取包含回车换行
		str_trim_crlf(setting_line);
		//加载配置项(将配置文件的配置项赋给变量)
		parseconf_load_setting(setting_line);
		//清空缓存
		memset(setting_line,0,sizeof(setting_line));
	}
	fclose(fp);
}
void parseconf_load_setting(const char* setting)
{
	while(isspace(*setting))
		setting++;
	char key[128] = {0};
	char value[128] = {0};
	str_split(setting,key,value,'=');
	if(strlen(value) == 0)
	{
		fprintf(stderr,"missing value in config file for:%s\n",key);
		exit(EXIT_FAILURE);
	}

	/* 将配置文件值，设置入变量                             */

	//遍历 字符串设置
	{
	const struct parseconf_str_setting *p_str_setting = parseconf_str_array;
	while(p_str_setting->p_setting_name != NULL)
	{
		if(strcmp(key,p_str_setting->p_setting_name) == 0)
		{
			//先前的值
			const char** p_cur_setting = p_str_setting->p_variable;
			if(*p_cur_setting)
				free((char*)*p_cur_setting);//const char*转换为char*会变成无符号
			*p_cur_setting = strdup(value);//创建堆内存，将value值考入并指向
			return;
		}
		p_str_setting++;
	}
	}

	//如果遍历该类型没有找到

	//遍历开关设置
	{
		const struct parseconf_bool_setting *p_bool_setting = parseconf_bool_array;
		while(p_bool_setting->p_setting_name != NULL)
		{
			if(strcmp(key,p_bool_setting->p_setting_name) == 0)
			{
				str_to_upper(value);
				if(strcmp(value,"YES") == 0
					|| strcmp(value,"TRUE") == 0
					|| strcmp(value,"1") == 0)
				{
					*(p_bool_setting->p_variable)  = 1;
				}
				else if(strcmp(value,"NO") == 0
						|| strcmp(value,"FALSE") == 0
						|| strcmp(value,"0") == 0)
				{
					*(p_bool_setting->p_variable)  = 0;
				}
				else //非法字符
				{
					fprintf(stderr,"bad value in bool config file for:%s\n",key);
					exit(EXIT_FAILURE);
				}
				return;
			}
			p_bool_setting++;
		}
		}

	//如果也非bool开关类型

	//遍历无符号整形配置，value有八进制与十进制形式
	{
		const struct parseconf_uint_setting *p_uint_setting = parseconf_uint_array;
		while(p_uint_setting->p_setting_name != NULL)
		{
			if(strcmp(key,p_uint_setting->p_setting_name) == 0)
			{
				if(value[0] == '0') //八进制以0开头
				{
					str_oct_to_uint(value);
					*(p_uint_setting->p_variable) = str_oct_to_uint(value);
				}
				else
				{
					*(p_uint_setting->p_variable) = atoi(value);
				}
				return;
			}
			p_uint_setting++;
		}
		}
}

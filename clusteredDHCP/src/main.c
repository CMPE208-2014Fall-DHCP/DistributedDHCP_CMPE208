
//CMPE 208 course project -- clustered DHCP servers     wei zhong 009433424

#include "config.h"
#include <pthread.h>
#include "getopt.h"

dhcp_config  dhcpconf;
dhcp_data    dhcpdata;
int heartbeat = 0;
int old_time;

struct option longopts[] = {
    {"logfile",       REQ_ARGU, NULL, 'l'},
    {"dbpath",        REQ_ARGU, NULL, 'p'},
	{"server",	      REQ_ARGU, NULL, 's'},
	{"loglevel",      REQ_ARGU, NULL, 'd'},
	{"display",		  NO_ARGU , NULL, 't'},
	{"daemon",        NO_ARGU,  NULL, 'D'}, 
};

void load_cfg_change()
{
	update_dhcp_stack(&dhcpconf);
}

void db_change()
{	
    int new_time = time(NULL);

	//update every 2 seconds
    if(old_time + 2 > new_time)
		return;
	
	heartbeat ++;
	old_time = new_time;
	write_heartbeat(dhcpconf.serverip, heartbeat, 1, 0, dhcpconf.hostname);
	load_cfg_change();
	load_lease(get_server(dhcpconf.serverip, &dhcpdata)); //maybe some node hand over lease to me
	mark_timeout_lease();
}

pthread_t start_task(void *ctrl, void * func)
{
    int             err = 0;
    pthread_attr_t  attr;
	pthread_t       thread;

    err = pthread_attr_init(&attr);
    if (err != 0)
        return(err);
			
    if((err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0)
		return err;
	
    err = pthread_create(&thread, &attr, func, ctrl);
	pthread_attr_destroy(&attr);
	if(err < 0 )
		return err;
	
    return thread;
}


void handle_sig(int sig_num)
{
    sig_num = sig_num;
    //exit_log();
}

void chk_para(dhcp_config *conf)
{
	if(!strlen(conf->db_path))
		strcpy(conf->db_path, DEFAULT_DBPATH);
	
    if(!strlen(conf->log_filename))
		strcpy(conf->log_filename, DEFAULT_DHCP_LOG);

    if(!conf->serverip) {
	    printf("must provide server ip address for this node!\n");
		exit(-1);
	}

	if(!validate_localip(conf->serverip)){
	    printf("server ip not found in this node!\n");
		exit(-1);
	}

    if(dhcpconf.loglevel < 0 || dhcpconf.loglevel > LOG_BUTT)    
		dhcpconf.loglevel = 0;

	gethostname(dhcpconf.hostname, sizeof(dhcpconf.hostname));
	old_time = time(NULL);
}

void load_para(int argc, char **argv)
{
    int  option;
    int  longindex;

	while ((option = getopt_long(argc, argv, "l:p:s:d:tD", longopts, &longindex)) != -1)
    {
        switch (option) {
			case 'l':
				strncpy(dhcpconf.log_filename, optarg, 256);
				break;
			case 'p':
				strncpy(dhcpconf.db_path, optarg, 256);
				break;
			case 's':
				dhcpconf.serverip = inet_addr(optarg);			
				break;
			case 'D':
				dhcpconf.daemon = 1;
				break;
			case 'd':
				dhcpconf.loglevel = atoi(optarg);
				break;
			case 't':
				dhcpconf.display = 1;
				break;				
			default:
				printf("not support\n");			
				exit(-1);
		}
	}
    
}

void register_sig()
{
    signal(SIGINT,  handle_sig);
    signal(SIGABRT, handle_sig);
    signal(SIGTERM, handle_sig);
    signal(SIGQUIT, handle_sig);
    signal(SIGTSTP, handle_sig);
    signal(SIGHUP,  handle_sig);
}

int main(int argc, char **argv)
{
	fd_set readfds;

	load_para(argc, argv);
    chk_para(&dhcpconf);
	init_log(&dhcpconf);
    init_db(&dhcpconf);
    init_dhcp_stack(&dhcpconf);
    //register_sig();

    while(1) {
		poll_sock(&dhcpdata, &readfds);
		db_change();
		dhcpserver(&readfds);
	}

	LOG_ERR("should never be here!\n");
}


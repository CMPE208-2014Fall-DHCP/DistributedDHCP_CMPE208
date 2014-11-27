

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "config.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string>
#include <sys/types.h>
#include <limits.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <signal.h>
#include <netdb.h>


typedef void           DHCP_VOID;
typedef char           DHCP_CHAR;             
typedef unsigned char  DHCP_UCHAR;
typedef unsigned char  DHCP_UINT8;
typedef unsigned short DHCP_UINT16;
typedef signed   short DHCP_INT16;
typedef unsigned int   DHCP_UINT32;
typedef signed   int   DHCP_INT32;
typedef int            DHCP_INT;
typedef unsigned int   DHCP_UINT;


#define IPQUAD(addr) \
((unsigned char *)&addr)[0], \
((unsigned char *)&addr)[1], \
((unsigned char *)&addr)[2], \
((unsigned char *)&addr)[3]

#define DEFAULT_DHCP_LOG    "/var/clusterdhcp.log" 
#define DEFAULT_DBPATH      "/mnt/glusterfs/" 
#define CFG_DBNAME          "cfg.db"
#define DYN_DBNAME          "lease.db"


#define CREATE_TABLE_XXX    "CREATE TABLE IF NOT EXISTS SCAN_RESULT (trans_id INTEGER, src INTEGER, dst INTEGER, seq INTEGER, router VARCHAR(60), in_port VARCHAR(60), out_port VARCHAR(60));"

#define    NO_ARGU      0
#define    REQ_ARGU     1


typedef enum _DB_DATA_TYPE
{
    CONFIG,      
    FIXEDIP,      
    LEASE,          
    ACTIVENODE,   
    DATA_BUTT,
}DB_DATA_TYPE;

typedef struct _db_op
{
	char    db_filename[256];
	char    table_create[512];
	int func;
	
}db_op;

typedef struct _db_data
{
	int     type;
    
	
}db_data;


typedef struct _dhcp_config
{
    int     serverip;
    int     daemon;
	char    db_path[256];
	
    //log
	int     display;
	int     loglevel;
	char    log_filename[256];

}dhcp_config;

typedef struct _server_sock
{
	int          sock;
	int          serverip;

}server_sock;

typedef struct _dhcp_data
{
	int          sock;
	int          server_num;
	server_sock  serverlist[16];
    DHCP_UINT16  lport;	
	int          maxFD;
	fd_set       reads;
	time_t       now_time;
}dhcp_data;

typedef enum _LOG_LEVEL
{
    DEBUG,      
    WARNING,      
    ERROR,          
    FATAL,   
    LOG_BUTT,
}LOG_LEVEL;

void log(int loglevel, int b, char *format, ...);
int validate_localip(int localip);
void init_log(dhcp_config *conf);
void exit_log();
void init_db(dhcp_config *conf);
void init_dhcp_stack(dhcp_config *conf);
void poll_sock(dhcp_data *dhcp, fd_set *readfds);
void dhcpserver(fd_set *readfds);
int db_getleasetime();
int db_getrouter();
int db_getnetmask();
int db_gethostname();
int init_dhcpsock(dhcp_config *conf, dhcp_data *dhcp);
int add_dhcpsock(int serverip, dhcp_data *dhcp);



#define LOG_DBG(fmt, ...)  log(DEBUG, fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)  log(ERROR, fmt, ##__VA_ARGS__)


#endif

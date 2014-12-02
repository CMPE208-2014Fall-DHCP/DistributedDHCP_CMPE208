

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <strings.h>
#include <string.h>
#include <stdarg.h>
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
#include "dhcplist.h"


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


#define IPQUAD(addr) ((unsigned char *)&addr)[0], ((unsigned char *)&addr)[1], ((unsigned char *)&addr)[2], ((unsigned char *)&addr)[3]

#define DEFAULT_DHCP_LOG    "/var/clusterdhcp.log" 
#define DEFAULT_DBPATH      "/mnt/glusterfs/" 
#define CFG_DBNAME          "cfg.db"
#define DYN_DBNAME          "lease.db"

#define TBL_CONFIG          "tbl_cfg"
#define TBL_FIXEDIP         "tbl_fixedip"
#define TBL_LEASE           "tbl_lease"
#define TBL_NODES           "tbl_nodes"


#define CREATE_TABLE_CFG      "CREATE TABLE IF NOT EXISTS tbl_cfg (key VARCHAR(32), val INTEGER);"
#define CREATE_TABLE_FIXEDIP  "CREATE TABLE IF NOT EXISTS tbl_fixedip (hw_addr VARCHAR(24), fixed_ip INTEGER, PRIMARY KEY (hw_addr));"
#define CREATE_TABLE_LEASE    "CREATE TABLE IF NOT EXISTS tbl_lease (lease_ip INTEGER, hw_addr VARCHAR(24), state INTEGER, timeout INTEGER, creator INTEGER, owner INTEGER, PRIMARY KEY (lease_ip));"
#define CREATE_TABLE_NODES    "CREATE TABLE IF NOT EXISTS tbl_nodes (node_name VARCHAR(32), server_ip INTEGER, state INTEGER, heartbeat INTEGER, takeover_ip INTEGER);"


#define    NO_ARGU      0
#define    REQ_ARGU     1

#define    TRUE         1
#define    FALSE        0


#define    GIVEUP       1
#define    TAKEOVER     0

typedef enum _DB_DATA_TYPE
{
    CONFIG,      
    FIXEDIP,      
    LEASE,          
    ACTIVENODE,   
    DATA_BUTT,
}DB_DATA_TYPE;

typedef struct _db_data
{
    int         type;
	int         datalen;
	
    DHCP_UINT8  data[256];
}db_data;

typedef struct _db_op
{
	char    db_filename[256];
	char    table_create[512];
	char    table_name[32];
	
	int     size;
	int     (*decode)(void *ptr, char **values);
}db_op;

typedef struct _tbl_cfg_tup
{
	char    name[32];
    int     val;
}tbl_cfg_tup;

typedef struct _tbl_fixedip_tup
{
	char    mac[24];
    int     ip;
}tbl_fixedip_tup;

typedef struct _tbl_lease_tup
{
	int     lease_ip;
	int     state;
	int     timeout;
	int     creator;
	int     owner;
	char    mac[24];
}tbl_lease_tup;

typedef struct _tbl_node_tup
{
	int     server_ip;
	int     state;
	int     heartbeat;
	int     takeover_ip;
	char    name[24];
}tbl_node_tup;



typedef struct _iprange
{
	DHCP_UINT32 start;  //host endian
	DHCP_UINT32 end;    //host endian
	int         router;  //network endian
	int         subnetmask;  //network endian
	int         leasetime;
}iprange;

typedef struct _server
{
	int          sock;
	int          master;
	int          serverip;  //network endian
	iprange      srv_ip_range;  
	int          leasenum;
	DHCP_UINT32  next_aval;  //host endian
	list_head    lease_list;    	
}server;

typedef struct _dhcp_config
{
    int     serverip;
    int     daemon;
	char    db_path[256];

	db_op   dbop[16];
    //log
	int     display;
	int     loglevel;
	char    log_filename[256];

	//server
	char    hostname[32];
	

}dhcp_config;

typedef struct _dhcp_data
{
	int          sock;
	int          server_num;
	server  serverlist[16];
    DHCP_UINT16  lport;	
	fd_set       reads;
	iprange      ip_range;
}dhcp_data;

typedef enum _LOG_LEVEL
{
    DEBUG,      
    WARNING,      
    ERROR,          
    FATAL,   
    LOG_BUTT,
}LOG_LEVEL;

typedef struct _ip_failover
{
	int   ip;
	int   state;
}ip_failover;

typedef struct _server_takeover_list
{
	pthread_mutex_t  mutex;
	int              num;
	ip_failover      list[16];
    
}server_takeover_list;


void logtext(int loglevel, char *format, ...);
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
int init_dhcpsock(dhcp_config *conf, dhcp_data *dhcp);
int add_dhcpsock(int serverip, dhcp_data *dhcp);
server *get_server_bysock(int sock, dhcp_data *dhcp);
server *get_server(int serverip, dhcp_data *dhcp);
db_op *get_dbop(int type);
int mem_rmvlease(int ip, server *node);
void update_dhcp_stack(dhcp_config *conf);
int write_heartbeat(int serverip, int heartbeat, int state, int takeoverop, char *name);



#define LOG_DBG(fmt, ...)  logtext(DEBUG, fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)  logtext(ERROR, fmt, ##__VA_ARGS__)


#endif

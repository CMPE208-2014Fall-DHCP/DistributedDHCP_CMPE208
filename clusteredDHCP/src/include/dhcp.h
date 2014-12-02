

#ifndef __DHCP_H__
#define __DHCP_H__

//dhcp protocl define
#define BOOTP_REQUEST  1
#define BOOTP_REPLY    2

#define DHCP_MESS_NONE       0
#define DHCP_MESS_DISCOVER   1
#define DHCP_MESS_OFFER      2
#define DHCP_MESS_REQUEST	 3
#define DHCP_MESS_DECLINE	 4
#define DHCP_MESS_ACK		 5
#define DHCP_MESS_NAK		 6
#define DHCP_MESS_RELEASE    7
#define DHCP_MESS_INFORM	 8


// DHCP OPTIONS
#define DHCP_OPTION_PAD						0
#define DHCP_OPTION_NETMASK          		1
#define DHCP_OPTION_ROUTER           		3
#define DHCP_OPTION_DNS              		6
#define DHCP_OPTION_HOSTNAME         		12
#define DHCP_OPTION_REQUESTEDIPADDR  		50
#define DHCP_OPTION_IPADDRLEASE      		51
#define DHCP_OPTION_OVERLOAD         		52
#define DHCP_OPTION_MESSAGETYPE      		53
#define DHCP_OPTION_SERVERID         		54
#define DHCP_OPTION_PARAMREQLIST     		55
#define DHCP_OPTION_MESSAGE          		56
#define DHCP_OPTION_MAXDHCPMSGSIZE   		57
#define DHCP_OPTION_RENEWALTIME      		58
#define DHCP_OPTION_REBINDINGTIME    		59
#define DHCP_OPTION_VENDORCLASSID    		60
#define DHCP_OPTION_CLIENTID         		61
#define DHCP_OPTION_USERCLASS        		77
#define DHCP_OPTION_RELAYAGENTINFO     		82
#define DHCP_OPTION_SUBNETSELECTION			118
#define DHCP_OPTION_BP_FILE					253
#define DHCP_OPTION_NEXTSERVER				254
#define DHCP_OPTION_END						255



typedef struct _dhcpop
{
	DHCP_UINT8 opt_code;
	DHCP_UINT8 size;
	DHCP_UINT8 value[256];
}dhcpop;

typedef struct _msg_control
{
	ulong cmsg_len;
	int cmsg_level;
	int cmsg_type;
	struct in_pktinfo pktinfo;
}msg_control;

typedef struct _dhcp_header
{
	DHCP_UINT8 bp_op;
	DHCP_UINT8 bp_htype;
	DHCP_UINT8 bp_hlen;
	DHCP_UINT8 bp_hops;
	DHCP_UINT32 bp_xid;
	DHCP_UINT16 bp_secs;
	DHCP_UINT8 bp_broadcast;
	DHCP_UINT8 bp_spare;
	DHCP_UINT32 bp_ciaddr;
	DHCP_UINT32 bp_yiaddr;
	DHCP_UINT32 bp_siaddr;
	DHCP_UINT32 bp_giaddr;
	DHCP_UINT8 bp_chaddr[16];
	char bp_sname[64];
	DHCP_UINT8 bp_file[128];
	DHCP_UINT8 bp_magic_num[4];
}dhcp_header;

typedef struct _dhcp_packet
{
	dhcp_header header;
	DHCP_UINT8 vend_data[1024 - sizeof(dhcp_header)];
}dhcp_packet;

typedef enum _LEASE_TYPE
{
    LEASE_FREE,
	LEASE_ALLOCATED,        
    LEASE_BUTT,
}LEASE_TYPE;

typedef struct _dhcplease
{
	list_head    list;    	
    int          leaseip;
	int          timeout;
	int          serverip;
	int          state; //
	int          fixedip;
	char         chaddr[24];
}dhcplease;

typedef struct _dhcprqst
{
	DHCP_UINT32 lease;
	union
	{
		char raw[sizeof(dhcp_packet)];
		dhcp_packet dhcpp;
	};
	char hostname[256];
	char chaddr[64];
	DHCP_UINT32 serverid;
	DHCP_UINT32 reqIP;
	int bytes;
	struct sockaddr_in remote;
	socklen_t sockLen;
	DHCP_UINT16 messsize;
	struct msghdr msg;
	struct iovec iov[1];
	msg_control msgcontrol;
	DHCP_UINT8 *vp;
	dhcpop agentOption;
	dhcpop clientId;
	dhcpop subnet;
	dhcpop vendClass;
	dhcpop userClass;
	DHCP_UINT32 subnetIP;
	DHCP_UINT32 targetIP;
	DHCP_UINT32 rebind;
	DHCP_UINT8 paramreqlist[256];
	DHCP_UINT8 opAdded[256];
	DHCP_UINT8 req_type;
	DHCP_UINT8 resp_type;
	time_t     now_time;
	server    *dhcpserver;
}dhcprqst;

dhcplease *get_mem_dhcplease(int ip, server *node);


#endif


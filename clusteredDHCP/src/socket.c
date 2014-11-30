
#include "config.h"

extern dhcp_config  dhcpconf;

void poll_sock(dhcp_data *dhcp, fd_set *readfds)
{
	struct timeval tv;

	FD_ZERO(readfds);
	tv.tv_sec  = 100000;
	tv.tv_usec = 0;

    *readfds = dhcp->reads;
	select(FD_SETSIZE, readfds, NULL, NULL, &tv);
}

server *get_server(int serverip, dhcp_data *dhcp)
{
    int i;
	
    if(dhcp->server_num == 0)
		return NULL;

    for(i = 0; i < dhcp->server_num; i++ ){
	    if(dhcp->serverlist[i].serverip != serverip)
			continue;
        return &dhcp->serverlist[i];
	}

	return NULL;
}

int validate_localip(int localip)
{
	struct sockaddr_in s_src;
	int sock, ret;

	sock = socket(AF_INET, SOCK_DGRAM, 0);  
	if (sock < 0) {	 
		printf("allocate socket fail!\n");
		return 0;	
	}	

	bzero(&s_src,sizeof(s_src));	
	s_src.sin_addr.s_addr = localip;
	s_src.sin_family = AF_INET;  
	ret = bind(sock, (struct sockaddr*) &s_src, sizeof(s_src));
	if(ret < 0) {
		printf("bind error, %s\n", strerror(errno)); 
		close(sock);
		return 0;
	}

    close(sock);
	return 1;
	
}

server *get_server_bysock(int sock, dhcp_data *dhcp)
{
    int i;
	
    if(dhcp->server_num == 0)
		return NULL;

    for(i = 0; i < dhcp->server_num; i++ ){
	    if(dhcp->serverlist[i].sock != sock)
			continue;
        return &dhcp->serverlist[i];
	}

	return NULL;
}

int add_dhcpsock(int serverip, dhcp_data *dhcp)
{
    int flag, ret, sock;
	struct sockaddr_in s_src;
	
	flag = TRUE;
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock < 0)
		return 0;
	
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));	
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&flag, sizeof(flag));	

	bzero(&s_src,sizeof(s_src));	
	s_src.sin_addr.s_addr = serverip;
	s_src.sin_family = AF_INET;  
	s_src.sin_port = htons(dhcp->lport);
	ret = bind(sock, (struct sockaddr*) &s_src, sizeof(s_src));
	if(ret < 0){
		LOG_ERR("bind error, %s\n", strerror(errno)); 
	    close(sock);
		return 0;
	}
		
    return sock;
}
 

int init_dhcpsock(dhcp_config *conf, dhcp_data *dhcp)
{
    int flag, ret;
	struct sockaddr_in s_src;

	flag = TRUE;
	dhcp->sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(dhcp->sock < 0)
		return 0;
	
	setsockopt(dhcp->sock, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));	
	setsockopt(dhcp->sock, SOL_IP, IP_PKTINFO, (char*)&flag, sizeof(flag));

	bzero(&s_src,sizeof(s_src));	
	s_src.sin_addr.s_addr = INADDR_ANY;
	s_src.sin_family = AF_INET;  
	s_src.sin_port = htons(dhcp->lport);
	ret = bind(dhcp->sock, (struct sockaddr*) &s_src, sizeof(s_src));
	if(ret < 0){
		LOG_ERR("bind error, %s\n", strerror(errno)); 
	    close(dhcp->sock);
		dhcp->sock = 0;
		return 0;
	}

    FD_SET(dhcp->sock, &dhcp->reads);
    return 1;
}



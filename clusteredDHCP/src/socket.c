
#include "config.h"
#include "linux/if_packet.h"
#include "linux/if_arp.h"
#include "linux/if_ether.h"

extern dhcp_config  dhcpconf;
DHCP_UCHAR   arp_msg[256];
DHCP_UCHAR   arp_sndmsg[256];

void poll_sock(dhcp_data *dhcp, fd_set *readfds)
{
	struct timeval tv;

	FD_ZERO(readfds);
	tv.tv_sec  = 1;
	tv.tv_usec = 0;

    *readfds = dhcp->reads;
	select(FD_SETSIZE, readfds, NULL, NULL, &tv);
}

int arp_brdsend(dhcp_data *dhcp, int type, int sip, DHCP_UINT8 *smac, int tip, DHCP_UINT8 *tmac)
{
	struct arphdr *arp;
    struct sockaddr_ll  ll_addr;
	DHCP_UINT8 *sha, *tha;
	DHCP_UINT8 *arp_ptr;
	int ret, len;

    arp = (struct arphdr *)(arp_sndmsg+14);
	arp->ar_hrd = htons(ARPHRD_ETHER); 		/* format of hardware address	*/
	arp->ar_pro = htons(ETH_P_IP); 		/* format of protocol address	*/
	arp->ar_hln = 6; 		/* length of hardware address	*/
	arp->ar_pln = 4; 		/* length of protocol address	*/
	arp->ar_op  = htons(type);			/* ARP opcode (command) 		*/

	arp_ptr = arp_sndmsg;
    memset(arp_ptr, 0xFF, 6);	
    memcpy(arp_ptr+6, dhcpconf.mac, 6);
	*((DHCP_UINT16 *)(arp_ptr+12)) = htons(ETH_P_ARP);
	
	arp_ptr = (DHCP_UINT8 *)(arp + 1); 
	sha = arp_ptr;
	memcpy(sha, smac, 6);
	arp_ptr += 6;
	memcpy(arp_ptr, &sip, 4); //src ip
	arp_ptr += 4;
	tha	= arp_ptr; //dst mac
	memcpy(tha, tmac, 6);
	arp_ptr += 6;
	memcpy(arp_ptr, &tip, 4); //dst ip
	arp_ptr += 4;
	len = arp_ptr - arp_sndmsg;

	ret = sendto(dhcp->arp_sock, arp_sndmsg, len, 0, NULL, 0);
	if(ret < 0)
		LOG_ERR("send arp packet fail!\n");
	return ret;
}

void arp_rsp(dhcp_data *dhcp, fd_set *readfds)
{
	struct arphdr *arp;
    struct sockaddr_ll  ll_addr;
	DHCP_UINT32 sip, tip;
	DHCP_UINT8 *sha, *tha;
	DHCP_UINT8 *arp_ptr;
	int ret, len;

	if (!(FD_ISSET(dhcp->arp_sock, readfds)))
		return;
	
	len = sizeof(ll_addr);
	ret = recvfrom(dhcp->arp_sock, arp_msg, 256, 0,(struct sockaddr*)&ll_addr, &len);
	if (ret <= 0) 
		return;

	arp = (struct arphdr *)(arp_msg+14);
	arp_ptr = (unsigned char *)(arp + 1); 
	sha	= arp_ptr; //src mac
	arp_ptr += 6;
	memcpy(&sip, arp_ptr, 4); //src ip
	arp_ptr += 4;
	tha	= arp_ptr; //dst mac
	arp_ptr += 6;
	memcpy(&tip, arp_ptr, 4); //dst ip

    //LOG_ERR("get arp: src ip %u.%u.%u.%u , target ip: %u.%u.%u.%u\n", IPQUAD(sip), IPQUAD(tip));
	if(sip == 0) //gratuitous arp, kernel will respond it
		return;

	get_arpresp(sip);
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

int validate_eth(dhcp_config *conf, char *ethname)
{
    int  sock_fd;
    struct ifreq  ifr;
    struct sockaddr    if_hwaddr;

    sock_fd = socket(PF_PACKET, SOCK_RAW, 0);
    if(sock_fd <= 0)
		return 0;
	
    memset(&ifr, 0, sizeof(ifr));
    strcpy (ifr.ifr_name, ethname);
    if (ioctl(sock_fd, SIOCGIFINDEX, &ifr) != 0){
        close(sock_fd);
		printf("not get if index for %s\n", ethname);
        return 0;
    }

    conf->if_index = ifr.ifr_ifindex;
	
    memset(&ifr, 0, sizeof(ifr));
    strcpy (ifr.ifr_name, ethname);
    if (ioctl(sock_fd, SIOCGIFHWADDR, &ifr) != 0)
    {
        close(sock_fd);
		printf("not get mac addr for %s\n", ethname);
        return 0;
    }
    
    if_hwaddr = ifr.ifr_hwaddr;
    memcpy(conf->mac, if_hwaddr.sa_data, 6); 
	
	close(sock_fd);
    return 1;
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

int arp_sock(dhcp_config *conf)
{
    struct sockaddr_ll  ll_addr;
    int sock;

	sock = socket(AF_PACKET, SOCK_RAW, 0);
    ll_addr.sll_family   = PF_PACKET;
    ll_addr.sll_protocol = (DHCP_UINT16)htons(ETH_P_ARP);
    ll_addr.sll_ifindex  = conf->if_index;
    //ll_addr.sll_hatype   = ARPHRD_ETHER;
    //ll_addr.sll_pkttype  = PACKET_OTHERHOST;
    //ll_addr.sll_halen    = ETH_ALEN;
    //ll_addr.sll_addr[6]  = 0x00; 
    //ll_addr.sll_addr[7]  = 0x00;
    
    if(bind(sock,(struct sockaddr *)&ll_addr, sizeof(ll_addr)) < 0)
    {
        close(sock);
        LOG_ERR("arp_sock: bind  err %s\n", strerror(errno)); 
        return 0;
    }

    return sock;
}

int delete_ip(int serverip, int index)
{
	char   cmd[256];

	memset(cmd, 0, 256);
	sprintf(cmd, "ifconfig %s:%d 0\n", dhcpconf.ethname, index);
	system(cmd);

}

int add_ip(int serverip, int index)
{
	char   cmd[256];
	
	memset(cmd, 0, 256);
	sprintf(cmd, "ifconfig %s:%d %u.%u.%u.%u netmask 255.255.255.0\n", 
		dhcpconf.ethname, index, IPQUAD(serverip));
	system(cmd);
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

	setsockopt(dhcp->sock, SOL_SOCKET, SO_BINDTODEVICE,conf->ethname, 32);

    FD_SET(dhcp->sock, &dhcp->reads);
    return 1;
}



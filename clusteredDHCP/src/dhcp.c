
#include "config.h"
#include "dhcp.h"

dhcprqst dhcpr;
extern dhcp_config  dhcpconf;
extern dhcp_data    dhcpdata;
server_takeover_list failover_list;

char *dhcp_msgname(DHCP_UINT8 type);
void analyze_options(dhcprqst *req);
void ack_lease(dhcprqst *req);
void dhcp_proc(int sock);


void dhcpserver(fd_set *readfds)
{
    int i = 0;
	server *server;
	
	for (i = 0; i < dhcpdata.server_num; i++){ 
	    server = &dhcpdata.serverlist[i];
		if (FD_ISSET(server->sock, readfds))
			dhcp_proc(server->sock);
	}

	if (FD_ISSET(dhcpdata.sock, readfds))
		dhcp_proc(dhcpdata.sock);

	return;
}

int dhcp_rcvmsg(int sock, dhcprqst *req)
{
	int msgflags = 0;
	int flags    = 0;
	DHCP_UINT32 addr;
	
    memset(req, 0, sizeof(dhcprqst));
	if (sock == dhcpdata.sock){
		req->iov[0].iov_base = req->raw;
		req->iov[0].iov_len = sizeof(dhcp_packet);
		req->msg.msg_iov = req->iov;
		req->msg.msg_iovlen = 1;
		req->msg.msg_name = &req->remote;
		req->msg.msg_namelen = sizeof(struct sockaddr_in);
		req->msg.msg_control = &req->msgcontrol;
		req->msg.msg_controllen = sizeof(msg_control);
		req->msg.msg_flags = msgflags;

		req->bytes = recvmsg(sock, &req->msg, flags);
		if (req->bytes <= 0)
			return 0;

		addr = req->msgcontrol.pktinfo.ipi_spec_dst.s_addr;
		if (!addr) {
			LOG_ERR("no dst ip found!\n");
			return 0;
		}
        req->dhcpserver = get_server(addr, &dhcpdata);
		if(!req->dhcpserver->master)
			req->dhcpserver = get_server(dhcpconf.serverip, &dhcpdata);
	}
	else
	{
	    req->dhcpserver = get_server_bysock(sock, &dhcpdata);
		req->sockLen = sizeof(req->remote);
		req->bytes = recvfrom(sock,
							  req->raw,
							  sizeof(dhcp_packet),
							  0,
							  (struct sockaddr*)&req->remote,
							  &req->sockLen);

		if (req->bytes <= 0)
			return 0;
	}

    if(req->dhcpserver == NULL){
		LOG_ERR("[server ip: %u.%u.%u.%u] not ours!\n", IPQUAD(addr));
		return 0;
	}

	return 1;
}

DHCP_UINT16 fUShort(void *raw)
{
	return ntohs(*((DHCP_UINT16*)raw));
}

DHCP_UINT32 fULong(void *raw)
{
	return ntohl(*((DHCP_UINT32*)raw));
}

DHCP_UINT32 fIP(void *raw)
{
	return (*((DHCP_UINT32*)raw));
}

DHCP_UINT8 pUShort(void *raw, DHCP_UINT16 data)
{
	*((DHCP_UINT16*)raw) = htons(data);
	return sizeof(DHCP_UINT16);
}

DHCP_UINT8 pULong(void *raw, DHCP_UINT32 data)
{
	*((DHCP_UINT32*)raw) = htonl(data);
	return sizeof(DHCP_UINT32);
}

DHCP_UINT8 pIP(void *raw, DHCP_UINT32 data)
{
	*((DHCP_UINT32*)raw) = data;
	return sizeof(DHCP_UINT32);
}


char *hex2String(char *target, DHCP_UINT8 *hex, DHCP_UINT8 bytes)
{
	char *dp = target;
	DHCP_UINT8 i;

	if (bytes)
		dp += sprintf(target, "%02x", *hex);
	else
		*target = 0;

	for (i = 1; i < bytes; i++)
			dp += sprintf(dp, ":%02x", *(hex + i));

	return target;
}

void dhcp_proc(int sock)
{
    int ret;
	
	if(!dhcp_rcvmsg(sock, &dhcpr))
		return;

	dhcpr.now_time = time(NULL);
	if (dhcpr.dhcpp.header.bp_op != BOOTP_REQUEST)
		return;

	hex2String(dhcpr.chaddr, dhcpr.dhcpp.header.bp_chaddr, dhcpr.dhcpp.header.bp_hlen);
    analyze_options(&dhcpr);

	switch(dhcpr.req_type) {
		case DHCP_MESS_NONE:
			LOG_ERR("not support dhcp msg relay yet!\n");
			return;
		case DHCP_MESS_DISCOVER:
			ret = dhcpdiscover(&dhcpr);
			break;
		case DHCP_MESS_REQUEST:
			ret = dhcprequest(&dhcpr);
			break;
		case DHCP_MESS_DECLINE:
			ret = dhcpdecline(&dhcpr);
			break;
		case DHCP_MESS_RELEASE:
			ret = dhcprelease(&dhcpr);
			break;
		case DHCP_MESS_INFORM:
			//LOG_ERR("not support dhcp inform yet!\n");
			break;
		default:
			LOG_ERR("unsupportted dhcp msg type[%d]!\n", dhcpr.req_type);
	}

	if(ret == 0)
		return;

	ack_lease(&dhcpr);
	return;
}

void pvdata(dhcprqst *req, dhcpop *op)
{
	char *ptr;

	if (!req->opAdded[op->opt_code] && ((req->vp - (DHCP_UINT8*)&req->dhcpp) + op->size < req->messsize))
	{
		if (op->opt_code == DHCP_OPTION_NEXTSERVER)
			req->dhcpp.header.bp_siaddr = fIP(op->value);
		else if (op->opt_code == DHCP_OPTION_BP_FILE)
		{
			if (op->size <= 128)
				memcpy(req->dhcpp.header.bp_file, op->value, op->size);
		}
		else if(op->size)
		{
			if (op->opt_code == DHCP_OPTION_IPADDRLEASE)
			{
				if (!req->lease || req->lease > fULong(op->value))
					req->lease = fULong(op->value);

				if (req->lease >= INT_MAX)
					req->lease = UINT_MAX;

				pULong(op->value, req->lease);
			}
			else if (op->opt_code == DHCP_OPTION_REBINDINGTIME)
				req->rebind = fULong(op->value);
			else if (op->opt_code == DHCP_OPTION_HOSTNAME)
			{
				memcpy(req->hostname, op->value, op->size);
				req->hostname[op->size] = 0;
				req->hostname[63] = 0;

				if ((ptr = strchr(req->hostname, '.'))!= NULL)
					*ptr = 0;
			}

			DHCP_UINT16 tsize = op->size + 2;
			memcpy(req->vp, op, tsize);
			(req->vp) += tsize;
		}
		req->opAdded[op->opt_code] = TRUE;
	}
}

void add_DWORD_option(dhcprqst *req, char type, DHCP_UINT32 val)
{
	dhcpop op;
	
	op.opt_code = type;
	op.size = 4;
	pIP(op.value, val);
	pvdata(req, &op);
}

void add_options(dhcprqst *req)
{
	dhcpop op;
	int i;

	if (req->req_type && req->resp_type)
	{
		op.opt_code = DHCP_OPTION_MESSAGETYPE;
		op.size = 1;
		op.value[0] = req->resp_type;
		pvdata(req, &op);
	}

    if(req->resp_type == DHCP_MESS_NAK)
		return;
	
	if (req->req_type && req->resp_type)
	{
		add_DWORD_option(req, DHCP_OPTION_ROUTER, req->dhcpserver->srv_ip_range.router);
		add_DWORD_option(req, DHCP_OPTION_SERVERID, req->dhcpserver->serverip);
		add_DWORD_option(req, DHCP_OPTION_IPADDRLEASE, req->dhcpserver->srv_ip_range.leasetime);
		add_DWORD_option(req, DHCP_OPTION_NETMASK, req->dhcpserver->srv_ip_range.subnetmask);
	}

	*(req->vp) = DHCP_OPTION_END;
}


void ack_lease(dhcprqst *req)
{ 
    int packSize;
	
	add_options(req);
	packSize = req->vp - (DHCP_UINT8*)&req->dhcpp;
	packSize++;

	if (req->req_type == DHCP_MESS_NONE)
		packSize = req->messsize;

	if (req->dhcpp.header.bp_broadcast || !req->remote.sin_addr.s_addr || req->reqIP){
		req->remote.sin_addr.s_addr = INADDR_BROADCAST;
	}

	req->remote.sin_port = htons(68);
	req->dhcpp.header.bp_op = BOOTP_REPLY;
	if (req->req_type == DHCP_MESS_DISCOVER && !req->dhcpp.header.bp_giaddr)
	{
		req->bytes = sendto(req->dhcpserver->sock,
							req->raw,
							packSize,
							MSG_DONTROUTE,
							(struct sockaddr*)&req->remote,
							sizeof(req->remote));
	}
	else
	{
		req->bytes = sendto(req->dhcpserver->sock,
							req->raw,
							packSize,
							0,
							(struct sockaddr*)&req->remote,
							sizeof(req->remote));
	}

	if (req->bytes <= 0)
		LOG_ERR("send dhcp msg %s for ip %u.%u.%u.%u fail!\n", 
			        dhcp_msgname(req->resp_type), IPQUAD(req->dhcpp.header.bp_yiaddr));

	
	LOG_ERR("[out: %s]: client: %s (mac: %s) lease ip: %u.%u.%u.%u \n", 
		dhcp_msgname(req->resp_type), req->hostname, req->chaddr, IPQUAD(req->dhcpp.header.bp_yiaddr));

	return;
}


int valid_range(server *node, int ip)
{
	if(ntohl(ip) >= node->srv_ip_range.start 
		&& ntohl(ip) <= node->srv_ip_range.end)
		return 1;
	return 0;
}

int add_mem_lease(dhcplease *lease, server *node)
{
	INIT_LIST_HEAD(&lease->list);
	list_add_tail(&lease->list, &node->lease_list);	
}

int handover_lease(server *master, int slave_ip)
{
    dhcplease *lease = (dhcplease *)malloc(256 * sizeof(dhcplease));
	int num = 256, i;

	db_server_leaselist(slave_ip, lease, &num, "creator");
	for(i = 0; i < num; i++)
	{
	    if(lease[i].serverip != master->serverip)
			continue;
		
	    lease[i].serverip = slave_ip;
		commit_lease(&lease[i], slave_ip);
  	}
}

int takeover_lease(server *master, int slave_ip)
{
    dhcplease *lease = (dhcplease *)malloc(256 * sizeof(dhcplease));
	int num = 256, i;

	db_server_leaselist(slave_ip, lease, &num, "creator");
	for(i = 0; i < num; i++)
	{
	    if(lease[i].serverip != slave_ip)  //someon takeover already
			LOG_ERR("someone takeover it already, force to takeover it anyway!\n");
		
	    lease[i].serverip = master->serverip;
		commit_lease(&lease[i], slave_ip);
  	}
}

int load_lease(server *node)
{
	dhcplease *lease_add;
    dhcplease *lease = (dhcplease *)malloc(256 * sizeof(dhcplease));
	int num = 256, i;
	int timeout = time(NULL);

	db_server_leaselist(node->serverip, lease, &num, "creator");
	for(i = 0; i < num; i++)
	{
        if(!valid_range(node, lease[i].leaseip)
			&& lease[i].leaseip != db_fixedip_query(lease[i].chaddr)){
			mem_rmvlease(lease[i].leaseip, node);
			db_lease_rmv_ip(lease[i].leaseip);
			continue;
		}
		//expired
        if(lease[i].timeout < timeout){
			lease[i].state = LEASE_FREE;
			commit_lease(&lease[i], node->serverip);
		}

		lease_add = get_mem_dhcplease(lease[i].leaseip, node);
		if(lease_add != NULL)
			continue;
		lease_add = (dhcplease *)malloc(sizeof(dhcplease));
		memcpy((void *)lease_add, (void *)&lease[i], sizeof(dhcplease));
		add_mem_lease(lease_add, node);     
	}
}

int mark_timeout_lease()
{
	server *node;
	dhcplease *lease = NULL;
	list_head* pos = NULL, *next = NULL;
	int timeout = time(NULL);
    int i;
	
    if(dhcpdata.server_num == 0)
		return 0;

    for(i = 0; i < dhcpdata.server_num; i++ ){
		node = &dhcpdata.serverlist[i];
	    if(!node->sock)
			continue;
		
		list_for_each_safe(pos, next, &node->lease_list) {
			lease = list_entry(pos, dhcplease, list);
			if(lease->timeout >= timeout) 
				continue;
			if(lease->state == LEASE_FREE) 
				continue;
			
			lease->state = LEASE_FREE;
			commit_lease(lease, node->serverip);
		}       
	}

	return 1;
}

int reclaim_db_lease(server *node)
{
	dhcplease *lease_one;
    dhcplease *lease = (dhcplease *)malloc(256 * sizeof(dhcplease));
	int num = 256, i;

	lease_one = lease;
	db_server_leaselist(node->serverip, lease, &num, "creator");
	for(i = 0; i < num; i++)
	{
        if(!valid_range(node, lease->leaseip)){
			mem_rmvlease(lease->leaseip, node);
			db_lease_rmv_ip(lease->leaseip);
    	}
		
	    lease_one = lease++;
	}

    return 1;  
}

void reclaim_mem_lease(server *node)
{
	dhcplease *lease = NULL;
	list_head* pos = NULL, *next = NULL;
	
	list_for_each_safe(pos, next, &node->lease_list) {
		lease = list_entry(pos, dhcplease, list);
		if(!lease)
			continue;
        if(!valid_range(node, lease->leaseip)){
			mem_rmvlease(lease->leaseip, node);
			db_lease_rmv_ip(lease->leaseip);
    	}
	}
	
	return;
}

int lease_reclaim(server *node)
{
	reclaim_mem_lease(node);
	reclaim_db_lease(node);
}

dhcplease *get_mem_leaseip(char *mac, server *node)
{	
	dhcplease *lease = NULL;
	list_head* pos = NULL, *next = NULL;
	
	list_for_each_safe(pos, next, &node->lease_list) {
		lease = list_entry(pos, dhcplease, list);
		if(!lease)
			continue;
		if(strncasecmp(lease->chaddr, mac, 24))
			continue;
		if(lease->timeout < time(NULL)) 
			lease->state = LEASE_FREE;
		return lease;	
	}
	
	return NULL;
}

dhcplease *get_mem_dhcplease(int ip, server *node)
{
	dhcplease *lease = NULL;
	list_head* pos = NULL, *next = NULL;
	
	list_for_each_safe(pos, next, &node->lease_list) {
		lease = list_entry(pos, dhcplease, list);
		if(!lease)
			continue;
		if(lease->leaseip != ip)
			continue;
		if(lease->timeout < time(NULL)) 
			lease->state = LEASE_FREE;
		return lease;	
	}
	
	return NULL;
}

int mem_rmvlease(int ip, server *node)
{
	dhcplease *lease;

    lease = get_mem_dhcplease(ip, node);
	if(lease == NULL)
		return 0;

	list_del(&lease->list);
	free(lease);
}

int db_mem_mark_rmvlease(int ip, server *node)
{
	dhcplease *lease;

    lease = get_mem_dhcplease(ip, node);
	if(lease == NULL)
		return 0;

	lease->state = LEASE_FREE;
}

int set_mem_leaseip_timeout(dhcplease *lease, int timeout)
{
	lease->state = LEASE_ALLOCATED;
	lease->timeout = time(NULL) + timeout;
	return lease->timeout;
}

dhcplease *lease_mem_new(dhcprqst *req, server *node, int ip)
{
	dhcplease *lease;
	int old = 1;

	if((lease = get_mem_dhcplease(ip, node)) == NULL){
		lease = (dhcplease *)malloc(sizeof(dhcplease));
		if(lease == NULL)
			return NULL;
		old = 0;
	}
	
	strncpy(lease->chaddr, req->chaddr, 24);
	lease->leaseip  = ip;
	lease->timeout  = 0;
	lease->serverip = node->serverip;
	lease->state    = LEASE_ALLOCATED;
	lease->fixedip  = 0;

	if(!old) {
		INIT_LIST_HEAD(&lease->list);
		list_add_tail(&lease->list, &node->lease_list);	
	}
	return lease;
}

int lease_mem_newip(server *node)
{
    int count = (int)(node->srv_ip_range.end - node->srv_ip_range.start + 1);
    int count2 = count;
	dhcplease *lease;

	
	while(count --) {
		if(node->next_aval > node->srv_ip_range.end)
			node->next_aval = node->srv_ip_range.start;
		if(get_mem_dhcplease(htonl(node->next_aval), node)){
			node->next_aval++;
		    continue;
		}
        return htonl(node->next_aval);
	}

    //let's force removing a lease with free state
	while(count2 --) {
		if(node->next_aval > node->srv_ip_range.end)
			node->next_aval = node->srv_ip_range.start;

		lease = get_mem_dhcplease(htonl(node->next_aval), node);
		if(lease->state != LEASE_FREE) {
			node->next_aval++;
			continue;
		}
			
        return lease->leaseip;
	}
	
    LOG_ERR("no ip resource available!\n");
	return 0;
}

int ip_release(dhcprqst *req, int ip)
{
	server *node;
	dhcplease  *lease, db_lease;

    node = req->dhcpserver;
	if((lease = get_mem_dhcplease(ip, node)) != NULL){
		db_mem_mark_rmvlease(ip, node);
		if(!db_lease_rmv_ip(ip))
			LOG_ERR("rmv ip fail: %u.%u.%u.%u\n", IPQUAD(ip));
		return 0;
    }

	//for failover 
	if(db_lease_queryip(ip, &db_lease) 
		&& db_lease.serverip == node->serverip){		
		if(!db_lease_rmv_ip(ip))
			LOG_ERR("rmv ip fail: %u.%u.%u.%u\n", IPQUAD(ip));
    }

	return 0;
}


int ip_alloc(dhcprqst *req)
{
	server *node;
	dhcplease  *lease, db_lease;
	int creator;
	int ip;

	node = req->dhcpserver;

	//check fixed ip
	ip = db_fixedip_query(req->chaddr);
    if(ip != 0){
		//deny ip lease to this mac
		if(ip == -1)
			return 0;

		if((lease = get_mem_dhcplease(ip, node)) != NULL) {
			LOG_ERR("fixed ip %u.%u.%u.%u has been allocated to an alive machine[mac: %s]\n", 
				IPQUAD(ip), lease->chaddr);
			return 0;
		}
		
		if((lease = get_mem_leaseip(req->chaddr, node)) != NULL){
			mem_rmvlease(lease->leaseip, node);
		}else if(((creator = db_lease_query(req->chaddr, &db_lease))>0)
			&& db_lease.serverip == node->serverip
			&& creator == node->serverip ){
			lease = &db_lease;
		}

		if(lease)
			db_lease_rmv_ip(lease->leaseip);
		
		if((lease = lease_mem_new(req, node, ip))!= NULL)	{
			set_mem_leaseip_timeout(lease, 30);
			commit_lease(lease, node->serverip); //store in database for presentation
			return lease->leaseip;
		}
		return 0;
	}

    //for dynamic ip allocation
	if((lease = get_mem_leaseip(req->chaddr, node)) != NULL){
		if(!db_lease_rmv_ip(lease->leaseip))
			LOG_ERR("rmv ip fail: %u.%u.%u.%u\n", IPQUAD(lease->leaseip));
		set_mem_leaseip_timeout(lease, 30);
		commit_lease(lease, node->serverip); //store in database for presentation
		return lease->leaseip;
    }

	//let's check db
	if(db_lease_query(req->chaddr, &db_lease) 
		&& db_lease.serverip == node->serverip
		&& valid_range(node, db_lease.leaseip)){
		if(!db_lease_rmv_ip(db_lease.leaseip))
			LOG_ERR("rmv ip fail: %u.%u.%u.%u\n", IPQUAD(db_lease.leaseip));
		add_mem_lease(&db_lease, node);
		set_mem_leaseip_timeout(&db_lease, 30);
		commit_lease(lease, node->serverip); //store in database for presentation
		return db_lease.leaseip;
    }

    //allocate new one
	ip = lease_mem_newip(node);
    if((lease = lease_mem_new(req, node, ip))!= NULL){
		set_mem_leaseip_timeout(lease, 30);
		commit_lease(lease, node->serverip); //store in database for presentation
		return lease->leaseip;
	}
	return 0;
}

int commit_lease(dhcplease *lease, int creator)
{
	return db_lease_add(lease->leaseip, lease->chaddr, lease->state, lease->timeout, lease->serverip, creator);
}

int ip_lease(dhcprqst *req)
{
	server *node;
	dhcplease  *lease, db_lease;

    node = req->dhcpserver;
	if((lease = get_mem_leaseip(req->chaddr, node)) != NULL){
		if(lease->state == LEASE_FREE) //lease timeout
			return 0;
		set_mem_leaseip_timeout(lease, req->lease);
		if(commit_lease(lease, node->serverip))
			return lease->leaseip;
		
		LOG_ERR("commit ip: %u.%u.%u.%u to db fail[server:%u.%u.%u.%u]!\n", 
			IPQUAD(lease->leaseip), IPQUAD(node->serverip));
		set_mem_leaseip_timeout(lease, 0);
		return 0;
    }

	//for failover 
	if(db_lease_query(req->chaddr, &db_lease) == node->serverip){
        if(db_lease.timeout < req->now_time) 
			return 0;
		
		db_lease.timeout = req->now_time + req->lease;
		if(commit_lease(&db_lease, node->serverip))
			return db_lease.leaseip;
		
		LOG_ERR("commit ip: %u.%u.%u.%u to db fail[server:%u.%u.%u.%u]!\n", 
			IPQUAD(db_lease.leaseip), IPQUAD(node->serverip));
    }

	return 0;
}

int dhcpdiscover(dhcprqst *req)
{
    DHCP_UINT32  ip;

    //failover ip cannot provide new lease
    if(!req->dhcpserver->master)
		return 0;
	
	ip = ip_alloc(req);
	if (!ip) {
        LOG_ERR("no ip available!\n");
		return 0;
	}
	
    req->dhcpp.header.bp_yiaddr = ip;
	req->resp_type = DHCP_MESS_OFFER;
	return 1;
}

int set_dhcp_ack(dhcprqst *req, DHCP_UINT32 ip)
{
	req->resp_type = DHCP_MESS_ACK;
	req->dhcpp.header.bp_yiaddr = ip;
	return 1;
}

int dhcp_request_iplease(DHCP_UINT32 req_ip, dhcprqst *req)
{
	DHCP_UINT32  iplease;

	if(!req_ip)
		return 0;

	iplease = (DHCP_UINT32)ip_lease(req);
	if(req_ip == iplease)
		return 1;

	return 0;
}

int dhcp_request_allocateIP(dhcprqst *req)
{	
	if(dhcp_request_iplease(req->reqIP, req))
		return set_dhcp_ack(req, req->reqIP);
	
	if(dhcp_request_iplease(req->dhcpp.header.bp_ciaddr, req))
		return set_dhcp_ack(req, req->dhcpp.header.bp_ciaddr);

	req->resp_type = DHCP_MESS_NAK;
	req->dhcpp.header.bp_yiaddr = 0;
	
	//LOG_ERR("DHCPREQUEST from Host %s (mac: %s) without Discover, NAKed\n", req->hostname, req->chaddr);
	return 1;

}

int dhcprequest(dhcprqst *req)
{
	if (req->serverid == req->dhcpserver->serverip){
	    return dhcp_request_allocateIP(req);
	}

	//may renew comes
	return dhcp_request_allocateIP(req);
}

int dhcpdecline(dhcprqst *req)
{
    int serverip;
	
    if (!req->dhcpp.header.bp_ciaddr)
		return 0;
	
	ip_release(req, req->dhcpp.header.bp_ciaddr);
	LOG_ERR("IP Address %u.%u.%u.%u declined by Host %s (mac:%s)\n", 
		   IPQUAD(req->dhcpp.header.bp_ciaddr), req->hostname, req->chaddr);
	return 0;
}

int dhcprelease(dhcprqst *req)
{
	int serverip;

    if (!req->dhcpp.header.bp_ciaddr)
		return 0;
	
	ip_release(req, req->dhcpp.header.bp_ciaddr);
    return 0;
}


void analyze_options(dhcprqst *req)
{
	dhcpop *op;
	char *ptr;
	DHCP_UINT8 *raw = req->dhcpp.vend_data;
	DHCP_UINT8 *rawEnd = raw + (req->bytes - sizeof(dhcp_header));
	int i;

	for (; raw < rawEnd && *raw != DHCP_OPTION_END;)
	{
		op = (dhcpop*)raw;
		switch (op->opt_code)
		{
			case DHCP_OPTION_PAD:
				raw++;
				continue;
			case DHCP_OPTION_PARAMREQLIST:
				for (i = 0; i < op->size; i++)
					req->paramreqlist[op->value[i]] = 1;
				break;
			case DHCP_OPTION_MESSAGETYPE:
				req->req_type = op->value[0];
				break;
			case DHCP_OPTION_SERVERID:
				req->serverid = fIP(op->value);
				break;
			case DHCP_OPTION_IPADDRLEASE:
				req->lease = fULong(op->value);
				break;
			case DHCP_OPTION_MAXDHCPMSGSIZE:
				req->messsize = fUShort(op->value);
				break;
			case DHCP_OPTION_REQUESTEDIPADDR:
				req->reqIP = fIP(op->value);
				break;
			case DHCP_OPTION_HOSTNAME:
				if (op->size && strcasecmp((char*)op->value, "(none)"))
				{
					memcpy(req->hostname, op->value, op->size);
					req->hostname[op->size] = 0;
					req->hostname[63] = 0;
					if (ptr = strchr(req->hostname, '.'))
						*ptr = 0;
				}
				break;
			case DHCP_OPTION_VENDORCLASSID:
				memcpy(&req->vendClass, op, op->size + 2);
				break;
			case DHCP_OPTION_USERCLASS:
				memcpy(&req->userClass, op, op->size + 2);
				break;
			case DHCP_OPTION_RELAYAGENTINFO:
				memcpy(&req->agentOption, op, op->size + 2);
				break;
			case DHCP_OPTION_CLIENTID:
				memcpy(&req->clientId, op, op->size + 2);
				break;
			case DHCP_OPTION_SUBNETSELECTION:
				memcpy(&req->subnet, op, op->size + 2);
				req->subnetIP = fULong(op->value);
				break;
			case DHCP_OPTION_REBINDINGTIME:
				req->rebind = fULong(op->value);
				break;
		}

		raw += 2;
		raw += op->size;
	}

	if (!req->subnetIP)
		req->subnetIP = req->dhcpserver->serverip;

	if (!req->messsize)
	{
		if (req->req_type == DHCP_MESS_NONE)
			req->messsize = req->bytes;
		else
			req->messsize = sizeof(dhcp_packet);
	}

	if (!req->lease || req->lease > req->dhcpserver->srv_ip_range.leasetime)
		req->lease = req->dhcpserver->srv_ip_range.leasetime;

	LOG_ERR("[in : %s]: client: %s (mac: %s) from interface %u.%u.%u.%u \n", 
		dhcp_msgname(req->req_type), req->hostname, req->chaddr, IPQUAD(req->dhcpserver->serverip));

	req->vp = req->dhcpp.vend_data;
	memset(req->vp, 0, sizeof(dhcp_packet) - sizeof(dhcp_header));
	return;
}

char *dhcp_msgname(DHCP_UINT8 type)
{
	if (type == DHCP_MESS_DISCOVER)
		return "DHCP DISCOVER";
	if (type == DHCP_MESS_REQUEST)
		return "DHCP REQUEST ";
	if (type == DHCP_MESS_DECLINE)
		return "DHCP DECLINE ";
	if (type == DHCP_MESS_RELEASE)
		return "DHCP RELEASE ";
	if (type == DHCP_MESS_INFORM)
		return "DHCP INFORM  ";
	if (type == DHCP_MESS_OFFER)
		return "DHCP OFFER   ";
	if (type == DHCP_MESS_ACK)
		return "DHCP ACK     ";
	if (type == DHCP_MESS_NAK)
		return "DHCP NAK     ";

	return "UNKOWN MSG";
}

int del_dhcpserver(int serverip, dhcp_data *dhcp)
{
    int i;
    if(dhcp->server_num == 0)
		return 0;

    for(i = 0; i < dhcp->server_num; i++ ){
	    if(dhcp->serverlist[i].serverip != serverip)
			continue;

        if(dhcp->serverlist[i].sock){
			close(dhcp->serverlist[i].sock);
	        FD_CLR(dhcp->serverlist[i].sock, &dhcp->reads);
    	}
		dhcp->serverlist[i].sock 	= 0;
		dhcp->serverlist[i].serverip = 0;
		dhcp->server_num --;
		break;
	}

	if(i != dhcp->server_num)
		memcpy(&dhcp->serverlist[i], &dhcp->serverlist[i+1], (dhcp->server_num-i)*sizeof(server));

	return 1;
}


void add_dhcpserver(dhcp_data *dhcp, int serverip, int sock, int master)
{
	server *node;

    if(dhcp->server_num >= 16)
		return;

	node = get_server(serverip, dhcp);
	if(!node){
		node = &dhcp->serverlist[dhcp->server_num];
		dhcp->server_num ++;
	}
	
	node->serverip = serverip;
	node->master   = master;
	INIT_LIST_HEAD(&node->lease_list);
	if(node->sock)
		LOG_ERR("server sock is alive, why overwrite?\n");

	if(sock <= 0)
		return;
	
	node->sock = sock;
	FD_SET(sock, &dhcp->reads);
}

void add_failover_ip(int ip, int state)
{
	ip_failover  *node;
    int i;
	
	pthread_mutex_lock(&failover_list.mutex);
	
    for(i = 0; i < failover_list.num; i++){
		node = &failover_list.list[i];
        if(ip == node->ip)	{
			node->state = state;
			goto exit;
    	}
	}

	failover_list.list[failover_list.num].ip = ip;
	failover_list.list[failover_list.num].state = state;
	failover_list.num ++;

exit:
	pthread_mutex_unlock(&failover_list.mutex); 
}

void init_dhcp_stack(dhcp_config *conf)
{
    FD_ZERO(&dhcpdata.reads);
	pthread_mutex_init(&failover_list.mutex, NULL);
	failover_list.num = 0;

	dhcpdata.lport = 67;
	if(!init_dhcpsock(conf, &dhcpdata)){
		LOG_ERR("init_dhcpsock fail!\n");
		exit(-1);
	}

    //each node has one server ip being master only
    add_dhcpserver(&dhcpdata, conf->serverip, add_dhcpsock(conf->serverip, &dhcpdata), TRUE);
	update_dhcp_stack(conf);
	load_lease(get_server(conf->serverip, &dhcpdata));
	printf("DHCP stack is ready, listen on port %d...\n", dhcpdata.lport);
}

void dhcp_takeover(int serverip, int state)
{
	if(state == GIVEUP){
		LOG_ERR("\nserver[%u.%u.%u.%u] goes up!\n\n", IPQUAD(serverip));
		//update owner 
		handover_lease(get_server(dhcpconf.serverip, &dhcpdata), serverip);
		del_dhcpserver(serverip, &dhcpdata);
	}else if(state == TAKEOVER){
		LOG_ERR("\nserver[%u.%u.%u.%u] goes down!\n\n", IPQUAD(serverip));
		//update owber 
		takeover_lease(get_server(dhcpconf.serverip, &dhcpdata), serverip);
		add_dhcpserver(&dhcpdata, serverip, add_dhcpsock(serverip, &dhcpdata), FALSE);
		db_node_mark_dead(serverip, dhcpconf.serverip);
	}else {
		LOG_ERR("\nserver[%u.%u.%u.%u] ", IPQUAD(serverip));
		LOG_ERR("unkown ip state %d!\n", state);
	}
}

void dhcp_server_takeover()
{
	ip_failover  *node;
	int i;
	
	pthread_mutex_lock(&failover_list.mutex);
	
	for(i = 0; i < failover_list.num; i++){
		node = &failover_list.list[i];
		dhcp_takeover(node->ip, node->state);
	}
	failover_list.num = 0;
	pthread_mutex_unlock(&failover_list.mutex); 
}

void load_neighbour_server(dhcp_data *dhcp, int *change)
{
	db_data db_query;
	tbl_node_tup *node;
    int node_num, count = 0;

	if(!db_nodes_list(&db_query))
		return;

	node_num = db_query.datalen/sizeof(tbl_node_tup);
	node = (tbl_node_tup *)db_query.data;
    while(node_num --) {
		if(!get_server(node->server_ip, dhcp)) {
			*change = 1;
			LOG_ERR("server %s [%u.%u.%u.%u] is alive from db view.\n", node->name, IPQUAD(node->server_ip));
			add_dhcpserver(dhcp, node->server_ip, 0, TRUE);
		}
		node++;
	}	

	//dont need to consider remove nodes, 
	//since all nodes' server ip will be takeover by active node, tuple for this ip is still there
}

server *get_serverip_greater(dhcp_data *dhcp, int ip)
{
	int i, index = 0;

	for(i = 0; i < dhcp->server_num; i++ ){
		if(dhcp->serverlist[i].serverip > ip
			&& dhcp->serverlist[index].serverip >= dhcp->serverlist[i].serverip)
			index = i;
	}

	return &dhcp->serverlist[index];
}

int get_master_num(dhcp_data *dhcp)
{
    int i, count = 0;
	
    for(i = 0; i < dhcp->server_num; i++ ){
	    if(dhcp->serverlist[i].master)
			count++;
	}

	return count;
}

void divide_range(dhcp_data *dhcp)
{
	int average, node_num, i, leastIP=0;
	DHCP_UINT32 ip_s, ip_e, start, end;
	iprange *range = &dhcp->ip_range;
    server *node;

	start = range->start;
	end   = range->end;
	node_num = get_master_num(dhcp);
	average  = (end - start)/node_num;
	ip_e = start - 1 ;

	//to simplify processing, not consider existing ip range for server so far
	for(i = 0; i < dhcp->server_num; i++ ){
		dhcp->serverlist[i].srv_ip_range.start = 0;
	}
	
	for(i = 0; i < dhcp->server_num; i++ ){
		ip_s = ip_e+1;
		ip_e = ip_s + average;
		if(ip_e > end)
			ip_e = end;
		node = get_serverip_greater(dhcp, leastIP);
		leastIP = node->serverip;
		if(!node->master)
			continue;
		if(node->srv_ip_range.start != 0)
			continue;
		node->srv_ip_range.start  = ip_s;
		node->srv_ip_range.end    = ip_e;
		node->srv_ip_range.router = range->router;
		node->srv_ip_range.subnetmask = range->subnetmask;
		node->srv_ip_range.leasetime  = range->leasetime;
		node->next_aval = ip_s;
	}
	 
}

void update_dhcp_stack(dhcp_config *conf)
{
	DHCP_UINT32     start, end;
	int     subnetmask;
	int     change;
	int     cur_node_num;
	iprange *range;
	
	range = &dhcpdata.ip_range;
 	start = ntohl(db_range_start());
 	end   = ntohl(db_range_end());
 	range->router = db_getrouter();
 	range->leasetime  =  db_getleasetime();
 	range->subnetmask = db_getnetmask();
	
	if(range->start != start || range->end != end )
		change = 1;
	
	range->start = start; 
	range->end   = end;
    dhcp_server_takeover();
	load_neighbour_server(&dhcpdata, &change);
    if(!change)
		return;
	
	divide_range(&dhcpdata);
	lease_reclaim(get_server(conf->serverip, &dhcpdata));
}


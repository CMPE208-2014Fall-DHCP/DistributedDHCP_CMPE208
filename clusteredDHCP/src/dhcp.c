
#include "config.h"
#include "dhcp.h"

dhcprqst dhcpr;
extern dhcp_config  dhcpconf;
extern dhcp_data    dhcpdata;


void dhcpserver(fd_set *readfds)
{
    int i = 0;
	server_sock server;
	
	for (i = 0; i < dhcpdata.server_num; i++){ 
	    server = dhcpdata.serverlist[i];
		if (FD_ISSET(server->sock, &readfds))
			dhcp_proc(server->sock);
	}

	if (FD_ISSET(dhcpdata.sock, &readfds))
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
		req->msg.msg_namelen = sizeof(sockaddr_in);
		req->msg.msg_control = &req->msgcontrol;
		req->msg.msg_controllen = sizeof(msg_control);
		req->msg.msg_flags = msgflags;

		req->bytes = recvmsg(sock, &req->msg, flags);
		if (req->bytes <= 0)
			return 0;

		addr = req->msgcontrol.pktinfo.ipi_spec_dst.s_addr;
		if (!addr)
			return 0;

        req->sockInd = get_server(addr);
	}
	else
	{
	    req->sockInd = get_server_bysock(sock);
		req->sockLen = sizeof(req->remote);
		req->bytes = recvfrom(sock,
							  req->raw,
							  sizeof(dhcp_packet),
							  0,
							  (sockaddr*)&req->remote,
							  &req->sockLen);

		if (req->bytes <= 0)
			return 0;
	}

    if(req->sockInd == NULL){
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

	if (bytes)
		dp += sprintf(target, "%02x", *hex);
	else
		*target = 0;

	for (DHCP_UINT8 i = 1; i < bytes; i++)
			dp += sprintf(dp, ":%02x", *(hex + i));

	return target;
}

void dhcp_proc(int sock)
{
    int ret;
	
	if(!dhcp_rcvmsg(sock, dhcpr))
		return;
	
	if (dhcpr.dhcpp.header.bp_op != BOOTP_REQUEST)
		return;

	hex2String(dhcpr.chaddr, dhcpr.dhcpp.header.bp_chaddr, dhcpr.dhcpp.header.bp_hlen);
    analyze_options(dhcpr);

	switch(dhcpr.req_type) {
		case DHCP_MESS_NONE:
			LOG_ERR("not support dhcp msg relay yet!\n");
			return;
		case DHCP_MESS_DISCOVER:
			ret = dhcpdiscover(dhcpr);
			break;
		case DHCP_MESS_REQUEST:
			ret = dhcprequest(dhcpr, dhcpdata);
			break;
		case DHCP_MESS_DECLINE:
			ret = dhcpdecline(dhcpr);
			break;
		case DHCP_MESS_RELEASE:
			ret = dhcprelease(dhcpr);
			break;
		case DHCP_MESS_INFORM:
			LOG_ERR("not support dhcp inform yet!\n");
			break;
		default:
			LOG_ERR("unsupportted dhcp msg type[%d]!\n", dhcpr.req_type);
	}

	if(ret == 0)
		return;

	ack_lease(dhcpr);
	return;
}

void pvdata(dhcprqst *req, dhcpop *op)
{
	//debug("pvdata");

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

				if (char *ptr = strchr(req->hostname, '.'))
					*ptr = 0;
			}

			DHCP_UINT16 tsize = op->size + 2;
			memcpy(req->vp, op, tsize);
			(req->vp) += tsize;
		}
		req->opAdded[op->opt_code] = true;
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

void addOptions(dhcprqst *req)
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

    if(!req->dhcpEntry || req->resp_type == DHCP_MESS_NAK)
		return;
	
	//strcpy(req->dhcpp.header.bp_sname, db_gethostname());
	if (req->req_type && req->resp_type)
	{
		add_DWORD_option(req, DHCP_OPTION_ROUTER, db_getrouter());
		add_DWORD_option(req, DHCP_OPTION_SERVERID, req->sockInd->serverip);
		add_DWORD_option(req, DHCP_OPTION_IPADDRLEASE, db_getleasetime());
		add_DWORD_option(req, DHCP_OPTION_NETMASK, db_getnetmask());
	}

	*(req->vp) = DHCP_OPTION_END;
}


void ack_lease(dhcprqst *req)
{ 
    int packSize;
	
	addOptions(req);
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
		req->bytes = sendto(req->sockInd->sock,
							req->raw,
							packSize,
							MSG_DONTROUTE,
							(sockaddr*)&req->remote,
							sizeof(req->remote));
	}
	else
	{
		req->bytes = sendto(req->sockInd->sock,
							req->raw,
							packSize,
							0,
							(sockaddr*)&req->remote,
							sizeof(req->remote));
	}

	if (req->bytes <= 0)
		LOG_ERR("send dhcp msg %s for ip %u.%u.%u.%u fail!\n", 
			        dhcp_msgname(req->resp_type), IPQUAD(req->dhcpp.header.bp_yiaddr));

	
	LOG_ERR("[out: %s]: send to %s (mac: %s) with lease ip: %u.%u.%u.%u \n", 
		dhcp_msgname(req->req_type), req->hostname, req->chaddr, IPQUAD(req->dhcpp.header.bp_yiaddr));

	return;
}

int back_pool(dhcplease *dhcpEntry, int time)
{
}

int ip_lease(dhcprqst *req)
{
	return inet_addr("192.168.100.177");	
}

DHCP_UINT32 resad(dhcprqst *req)
{
	return inet_addr("192.168.100.177");	
}


int dhcpdiscover(dhcprqst *req)
{
    DHCP_UINT32  ip;

	ip = resad(req);
	if (!ip)
		return 0;

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

int dhcp_request_iplease(DHCP_UINT32 req_ip, dhcprqst *req, time_t *now)
{
	if(!req_ip)
		return 0;
	
	if(req_ip == ip_lease(req)
		 && req->dhcpEntry->expiry > now){
		return 1;
    }

	return 0;
}

int dhcp_request_allocateIP(dhcprqst *req, dhcp_data *dhcpdata)
{
	if (req->server != req->sockInd->serverip)
		return 0;
	
	if(dhcp_request_iplease(req->reqIP, req, dhcpdata->now_time))
		return set_dhcp_ack(req, req->reqIP);
	
	if(dhcp_request_iplease(req->dhcpp.header.bp_ciaddr, req, dhcpdata->now_time))
		return set_dhcp_ack(req, req->dhcpp.header.bp_ciaddr);
	
	req->resp_type = DHCP_MESS_NAK;
	req->dhcpp.header.bp_yiaddr = 0;
	
	LOG_ERR("DHCPREQUEST from Host %s (mac: %s) without Discover, NAKed", req->hostname, req->chaddr);
	return 1;

}

int dhcprequest(dhcprqst *req, dhcp_data *dhcpdata)
{
	if (req->server){
	    return dhcp_request_allocateIP(req, dhcpdata);
	}

	LOG_ERR("Not our dhcp requst, try to determine whether the allocated IP managed by us!\n");
	return dhcp_request_allocateIP(req, dhcpdata);
}

int dhcpdecline(dhcprqst *req)
{
    if (!req->dhcpp.header.bp_ciaddr)
		return;
	
	if (ip_lease(req) == req->dhcpp.header.bp_ciaddr)
	{	
		req->dhcpEntry->ip = 0;
		req->dhcpEntry->expiry = INT_MAX;
		req->dhcpEntry->display = false;
		req->dhcpEntry->local = false;
	
		LOG_ERR("IP Address %u.%u.%u.%u declined by Host %s (mac:%s), locked", 
			   IPQUAD(req->dhcpp.header.bp_ciaddr), req->hostname, req->chaddr);
	}

	return 0;
}

int dhcprelease(dhcprqst *req)
{
    if (!req->dhcpp.header.bp_ciaddr)
		return;
	
	if (ip_lease(req) == req->dhcpp.header.bp_ciaddr)
	{
		req->dhcpEntry->display = false;
		req->dhcpEntry->local = false;
		back_pool(req->dhcpEntry, 0);
	}
	
    return 0;
}


void analyze_options(dhcprqst *req)
{
	dhcpop *op;
	char *ptr;
	DHCP_UINT8 *raw = req->dhcpp.vend_data;
	DHCP_UINT8 *rawEnd = raw + (req->bytes - sizeof(dhcp_header));

	for (; raw < rawEnd && *raw != DHCP_OPTION_END;)
	{
		op = (dhcpop*)raw;
		switch (op->opt_code)
		{
			case DHCP_OPTION_PAD:
				raw++;
				continue;
			case DHCP_OPTION_PARAMREQLIST:
				for (int ix = 0; ix < op->size; ix++)
					req->paramreqlist[op->value[ix]] = 1;
				break;
			case DHCP_OPTION_MESSAGETYPE:
				req->req_type = op->value[0];
				break;
			case DHCP_OPTION_SERVERID:
				req->server = fIP(op->value);
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
		req->subnetIP = req->sockInd->server;

	if (!req->messsize)
	{
		if (req->req_type == DHCP_MESS_NONE)
			req->messsize = req->bytes;
		else
			req->messsize = sizeof(dhcp_packet);
	}

	LOG_ERR("[in: %s]: receive request for %s (mac: %s) from interface %u.%u.%u.%u \n", 
		dhcp_msgname(req->req_type), req->hostname, req->chaddr, IPQUAD(req->sockInd->server));

	req->vp = req->dhcpp.vend_data;
	memset(req->vp, 0, sizeof(dhcp_packet) - sizeof(dhcp_header));
	return;
}

char *dhcp_msgname(DHCP_UINT8 type)
{
	if (type == DHCP_MESS_DISCOVER)
		return "DHCP DISCOVER"
	if (type == DHCP_MESS_REQUEST)
		return "DHCP REQUEST"
	if (type == DHCP_MESS_DECLINE)
		return "DHCP DECLINE"
	if (type == DHCP_MESS_RELEASE)
		return "DHCP RELEASE"
	if (type == DHCP_MESS_INFORM)
		return "DHCP INFORM";

	return "UNKOWN MSG";
}

void init_dhcp_stack(dhcp_config *conf)
{
    FD_ZERO(&dhcpdata.reads);
	
	if(!init_dhcpsock(conf, dhcpdata)){
		LOG_ERR("init_dhcpsock fail!\n");
		exit(-1);
	}

    add_dhcpsock(conf.serverip, dhcpdata);	

	
}




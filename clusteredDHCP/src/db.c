
#include "config.h"
#include "sqlite3.h"
#include "dhcp.h"

extern dhcp_config  dhcpconf;

void db_add_op(dhcp_config *conf, 
	int (*decode)(void *ptr, char **values),
	int type, char *tbl_name, char *crt_sql, char *name, int size)
{
    db_data init_tbl;
	
	if(type >= 16)
		return;
	
	strncpy(conf->dbop[type].db_filename, conf->db_path, 256);
	strncat(conf->dbop[type].db_filename, tbl_name, 256);
	strncpy(conf->dbop[type].table_create, crt_sql, 512);
	strncpy(conf->dbop[type].table_name, name, 32);
	conf->dbop[type].size   = size;
	conf->dbop[type].decode = decode;

    init_tbl.type    = type;
	init_tbl.datalen = 0;

	db_sqlite_exec(&init_tbl, 0, NULL);
}

int cfg_decode(void *ptr, char **values)
{
    tbl_cfg_tup *cfg = (tbl_cfg_tup *)ptr;
	
    if (NULL != values[0])
    {
        strncpy(cfg->name, values[0], 32);
    }
    if (NULL != values[1])
    {
		cfg->val = atoi(values[1]);
    }

	return 1;
}

int cfg_add_val(char *name, int val)
{
    db_data data;
	db_op  *op;
	char cmdbuff[256];

	memset(cmdbuff, 0, 256);
	data.type = CONFIG;	
	op  = get_dbop(data.type);
	snprintf(cmdbuff, 256, "INSERT INTO %s values (\"%s\", %d);", op->table_name, name, val);

	return db_sqlite_exec(&data, 0, cmdbuff);
}

int cfg_read_val(char *name, int *val)
{
    db_data data;
	tbl_cfg_tup *tuple = (tbl_cfg_tup *)&data.data;
	db_op  *op;
	char cmdbuff[256];

	memset(cmdbuff, 0, 256);
	data.type    = CONFIG;
	data.datalen = sizeof(tbl_cfg_tup);
	op  = get_dbop(data.type);
	snprintf(cmdbuff, 256, "SELECT * FROM %s WHERE key = '%s';", op->table_name, name);

	if(!db_sqlite_exec(&data, 1, cmdbuff) || data.datalen == 0)
		return 0;
	
	*val = tuple->val;
	return 1;
}

int db_getleasetime()
{
    int time = 0;

    cfg_read_val("lease_time", &time); 
	if(time == 0)
		return 600;
	
	return time;
}
int db_getrouter()
{
    int router = 0;

    cfg_read_val("router", &router); 
	return router;
}
int db_getnetmask()
{
    int mask = 0;

    cfg_read_val("subnetmask", &mask); 
	return mask;
}

int db_range_start()
{
    int start = 0;

    cfg_read_val("range_start", &start); 
	return start;
}
int db_range_end()
{
    int end = 0;

    cfg_read_val("range_end", &end); 
	return end;
}

int fixedip_decode(void *ptr, char **values)
{
    tbl_fixedip_tup *fixedip = (tbl_fixedip_tup *)ptr;
	
    if (NULL != values[0])
    {
        strncpy(fixedip->mac, values[0], 24);
    }
    if (NULL != values[1])
    {
		fixedip->ip = atoi(values[1]);
    }

	return 1;
}

int fixedip_insert_tuple(char *mac, int ip)
{
    db_data data;
	db_op  *op;
	char cmdbuff[256];

	memset(cmdbuff, 0, 256);
	data.type    = FIXEDIP;
	op  = get_dbop(data.type);
	snprintf(cmdbuff, 256, "INSERT INTO %s values (\"%s\", %d)", op->table_name, mac, ip);

	return db_sqlite_exec(&data, 0, cmdbuff);
}

int db_fixedip_query(char *mac)
{
    db_data data;
	tbl_fixedip_tup *tuple = (tbl_fixedip_tup *)&data.data;
	db_op  *op;
	char cmdbuff[256];

	memset(cmdbuff, 0, 256);
	data.type    = FIXEDIP;
	data.datalen = sizeof(tbl_fixedip_tup);
	op  = get_dbop(data.type);
	snprintf(cmdbuff, 256, "SELECT * FROM %s WHERE hw_addr = '%s';", op->table_name, mac);

	if(!db_sqlite_exec(&data, 1, cmdbuff)|| data.datalen == 0)
		return 0;
	
    //if found but zero ip, then return -1
    if(tuple->ip == 0)
		return -1;
	
	return tuple->ip;
}

int db_node_mark_dead(int serverip, int takeover_ip)
{
    db_data data;
	tbl_node_tup *tuple = (tbl_node_tup *)&data.data;
	db_op  *op;
	char cmdbuff[256];

	memset(cmdbuff, 0, 256);
	data.type    = ACTIVENODE;
	data.datalen = sizeof(tbl_node_tup);
	op  = get_dbop(data.type);
	snprintf(cmdbuff, 256, "SELECT * FROM %s WHERE server_ip = %d;", op->table_name, serverip);

	if(!db_sqlite_exec(&data, 1, cmdbuff))
		return 0;
	
	memset(cmdbuff, 0, 256);
	if(data.datalen == 0){
		LOG_ERR("node[%u.%u.%u.%u] info in db gone!\n", IPQUAD(serverip));
	}else {
        snprintf(cmdbuff, 256, "update %s set node_name = '%s', state = %d, heartbeat = %d, takeover_ip = %d where server_ip = %d;",
                   op->table_name, tuple->name, 0, tuple->heartbeat, takeover_ip, serverip);

	}

	return db_sqlite_exec(&data, 0, cmdbuff);
}

int write_heartbeat(int serverip, int heartbeat, int state, int takeoverop, char *name)
{
    tbl_node_tup tup;

	tup.server_ip = serverip;
	tup.state     = state;;
	tup.heartbeat = heartbeat;
	tup.takeover_ip = takeoverop;
	strncpy(tup.name, name, 24);

	return nodes_update_tuple(&tup);
}

int nodes_decode(void *ptr, char **values)
{
    tbl_node_tup *node = (tbl_node_tup *)ptr;

	strncpy(node->name, values[0], 24);
	node->server_ip  = atoi(values[1]);
	node->state      = atoi(values[2]); 
	node->heartbeat  = atoi(values[3]);
	node->takeover_ip= atoi(values[4]);
	return 1;
}

int nodes_update_tuple(tbl_node_tup *tup)
{
    db_data data;
	tbl_node_tup *tuple = (tbl_node_tup *)&data.data;
	db_op  *op;
	char cmdbuff[256];

	memset(cmdbuff, 0, 256);
	data.type    = ACTIVENODE;
	data.datalen = sizeof(tbl_node_tup);
	op  = get_dbop(data.type);
	snprintf(cmdbuff, 256, "SELECT * FROM %s WHERE server_ip = %d;", op->table_name, tup->server_ip);

	if(!db_sqlite_exec(&data, 1, cmdbuff))
		return 0;
	
	memset(cmdbuff, 0, 256);
	if(data.datalen == 0){
		snprintf(cmdbuff, 256, "INSERT INTO %s values (\"%s\", %d, %d, %d, %d)",
				   op->table_name, tup->name, tup->server_ip, tup->state, tup->heartbeat, tup->takeover_ip);
	}else {
        snprintf(cmdbuff, 256, "update %s set node_name = '%s', state = %d, heartbeat = %d, takeover_ip = %d where server_ip = %d;",
                   op->table_name, tup->name, tup->state, tup->heartbeat, tup->takeover_ip, tup->server_ip);

	}

	return db_sqlite_exec(&data, 0, cmdbuff);
}

int db_nodes_list(db_data *data)
{
	tbl_node_tup *tuple = (tbl_node_tup *)&data->data;
	db_op  *op;
	char cmdbuff[256];

	memset(cmdbuff, 0, 256);
	data->type    = ACTIVENODE;
	data->datalen = 256;
	op  = get_dbop(data->type);
	snprintf(cmdbuff, 256, "SELECT * FROM %s;", op->table_name);

	if(!db_sqlite_exec(data, 1, cmdbuff)|| data->datalen == 0)
		return 0;
	
    return 1;
}

int lease_decode(void *ptr, char **values)
{
    tbl_lease_tup *lease = (tbl_lease_tup *)ptr;
	
	lease->lease_ip = atoi(values[0]);
	lease->state    = atoi(values[2]);
	lease->timeout  = atoi(values[3]);
	lease->creator  = atoi(values[4]);
	lease->owner    = atoi(values[5]);
	strncpy(lease->mac, values[1], 24);
	return 1;
}

void init_db(dhcp_config *conf)
{
	db_add_op(conf, cfg_decode, CONFIG, CFG_DBNAME, CREATE_TABLE_CFG, TBL_CONFIG, sizeof(tbl_cfg_tup));
	db_add_op(conf, fixedip_decode, FIXEDIP, CFG_DBNAME, CREATE_TABLE_FIXEDIP, TBL_FIXEDIP, sizeof(tbl_fixedip_tup));
	db_add_op(conf, lease_decode, LEASE, DYN_DBNAME, CREATE_TABLE_LEASE, TBL_LEASE, sizeof(tbl_lease_tup));
	db_add_op(conf, nodes_decode, ACTIVENODE, DYN_DBNAME, CREATE_TABLE_NODES, TBL_NODES, sizeof(tbl_node_tup));

    //configured by user through web portal
	//cfg_add_val("range_start", inet_addr("192.168.100.30"));
	//cfg_add_val("range_end",   inet_addr("192.168.100.50"));
	//cfg_add_val("subnetmask",  inet_addr("255.255.255.0"));
	//cfg_add_val("router",      inet_addr("192.168.100.130"));
	//cfg_add_val("lease_time",  600);
}

int lease_insert_tuple(tbl_lease_tup *tup)
{
    db_data data;
	db_op  *op;
	char cmdbuff[256];

	memset(cmdbuff, 0, 256);
	data.type    = LEASE;
	op  = get_dbop(data.type);
	snprintf(cmdbuff, 256, "INSERT INTO %s values (%d, \"%s\", %d, %d, %d, %d)",
			   op->table_name, tup->lease_ip, tup->mac, tup->state, tup->timeout, tup->creator, tup->owner);

	return db_sqlite_exec(&data, 0, cmdbuff);
}

int db_lease_rmv_ip(int ip)
{
    db_data data;
	db_op  *op;
	char cmdbuff[256];

	memset(cmdbuff, 0, 256);
	data.type    = LEASE;
	op  = get_dbop(data.type);
	snprintf(cmdbuff, 256, "DELETE FROM %s WHERE lease_ip = %d", op->table_name, ip);
	
	return db_sqlite_exec(&data, 0, cmdbuff);
}

int db_server_leaselist(int serverip, dhcplease *lease, int *num, char *name)
{
    db_data data, *pdata = &data;
	tbl_lease_tup *tuple;
	db_op  *op;
	char cmdbuff[256];
    int i;
	
	memset(cmdbuff, 0, 256);
	data.type    = LEASE;
	data.datalen = 256;
	if((*num) * sizeof(tbl_lease_tup) > 256){
		pdata = (db_data *)malloc(sizeof(db_data) - 256 + (*num) * sizeof(tbl_lease_tup));
		pdata->type	   = LEASE;
		pdata->datalen = (*num) * sizeof(tbl_lease_tup);
	}
	
	op  = get_dbop(pdata->type);
	snprintf(cmdbuff, 256, "SELECT * FROM %s WHERE %s = %d;", op->table_name, name, serverip);

	if(!db_sqlite_exec(pdata, 1, cmdbuff)|| pdata->datalen == 0){
		if((*num) * sizeof(tbl_lease_tup) > 256)
	        free(pdata);
		*num = 0;
		return 0;
	}

	tuple = (tbl_lease_tup *)pdata->data;
    *num = pdata->datalen / sizeof(tbl_lease_tup);
    for(i = 0; i < *num; i++){
		lease[i].leaseip  = tuple[i].lease_ip;
		lease[i].timeout  = tuple[i].timeout;
		lease[i].serverip = tuple[i].owner;
		lease[i].state	  = tuple[i].state;
		strncpy(lease[i].chaddr, tuple[i].mac, 24);
	}
	
    return 1;
}

int db_lease_add(int ip, char *mac, int state, int timeout, int serverip, int creator)
{
    db_data data;
	tbl_lease_tup *tuple = (tbl_lease_tup *)&data.data;
	db_op  *op;
	char cmdbuff[256];

	memset(cmdbuff, 0, 256);
	data.type    = LEASE;
	data.datalen = sizeof(tbl_lease_tup);
	op  = get_dbop(data.type);
	snprintf(cmdbuff, 256, "SELECT * FROM %s WHERE lease_ip = %d;", op->table_name, ip);
	if(!db_sqlite_exec(&data, 1, cmdbuff))
		return 0;

	
	memset(cmdbuff, 0, 256);
	if(data.datalen == 0){
		snprintf(cmdbuff, 256, "INSERT INTO %s values (%d, \"%s\", %d, %d, %d, %d)",
				   op->table_name, ip, mac, state, timeout, creator, serverip);
	}else {
	    //different server ip
		if(tuple->owner != serverip) {
			LOG_ERR("lease ip: %u.%u.%u.%u is occupied by other server[%u.%u.%u.%u]. local server[%u.%u.%u.%u]\n",
				IPQUAD(ip), IPQUAD(tuple->owner), IPQUAD(serverip));
			return 0;
		}
        snprintf(cmdbuff, 256, "update %s set hw_addr = '%s', state = %d, timeout = %d, creator = %d, owner = %d where lease_ip = %d;",
                   op->table_name, mac, state, timeout, creator, serverip, ip);

	}

	return db_sqlite_exec(&data, 0, cmdbuff);
}

int db_lease_query(char *mac, dhcplease *lease)
{
    db_data data;
	tbl_lease_tup *tuple = (tbl_lease_tup *)&data.data;
	db_op  *op;
	char cmdbuff[256];

	memset(cmdbuff, 0, 256);
	data.type    = LEASE;
	data.datalen = sizeof(tbl_lease_tup);
	op  = get_dbop(data.type);
	snprintf(cmdbuff, 256, "SELECT * FROM %s WHERE hw_addr = '%s';", op->table_name, mac);

	if(!db_sqlite_exec(&data, 1, cmdbuff)|| data.datalen == 0)
		return 0;
	
	lease->leaseip  = tuple->lease_ip;
	lease->timeout  = tuple->timeout;
	lease->serverip = tuple->owner;
	lease->state    = tuple->state;
	strncpy(lease->chaddr, tuple->mac, 24);
	return tuple->creator;
}

int db_lease_queryip(int ip, dhcplease *lease)
{
    db_data data;
	tbl_lease_tup *tuple = (tbl_lease_tup *)&data.data;
	db_op  *op;
	char cmdbuff[256];

	memset(cmdbuff, 0, 256);
	data.type    = LEASE;
	data.datalen = sizeof(tbl_lease_tup);
	op  = get_dbop(data.type);
	snprintf(cmdbuff, 256, "SELECT * FROM %s WHERE lease_ip = %d;", op->table_name, ip);

	if(!db_sqlite_exec(&data, 1, cmdbuff)|| data.datalen == 0)
		return 0;
	
	lease->leaseip  = tuple->lease_ip;
	lease->timeout  = tuple->timeout;
	lease->serverip = tuple->owner;
	lease->state    = tuple->state;
	strncpy(lease->chaddr, tuple->mac, 24);
	return 1;
}


db_op *get_dbop(int type)
{
	if(type >= 16)
		return NULL;

	return &dhcpconf.dbop[type];
}

int db_sqlite_exec(db_data *data, int read, char *cmdbuff)
{
	sqlite3 *db = NULL;
	char *sqlerr = NULL;
	int ret, ok = 1, i, max_row;
	db_op *op;
	char **dbResult = NULL;
    int nRow = 0;
    int nColumn  = 0;


	if(DATA_BUTT == data->type) {
		LOG_ERR("get unexpected data type!\n");
		return 0;
	}

	op = get_dbop(data->type);
	if(op == NULL) {
		LOG_ERR("not installed for type %d!\n", data->type);
		return 0;
	}
	
	ret = sqlite3_open(op->db_filename, &db);
	if (ret != 0 || db == NULL){
		LOG_ERR("open %s fail.\n", op->db_filename);
		return 0;
	}

	if (sqlite3_busy_timeout(db, 2000) != 0){
		ok = 0;
		goto close_db;
	}

	ret = sqlite3_exec(db, op->table_create,NULL, NULL, &sqlerr);
	if (ret != 0) {
		LOG_DBG("sql fail: %s . err=%s\n", op->table_create, sqlerr );
		ok = 0;
		goto close_db;
	}

    if(!cmdbuff || !cmdbuff[0])
		goto close_db;

	if(read == 0){
		if(sqlite3_exec(db, (const char *)cmdbuff, NULL, NULL, &sqlerr) != 0){
			ok = 0;
			LOG_DBG("sql fail: %s . err=%s\n", cmdbuff, sqlerr );
		}
		goto close_db;
	}

    //read copy data
    if(sqlite3_get_table(db, cmdbuff, &dbResult, &nRow, &nColumn, &sqlerr)!=0
		|| NULL == dbResult){
		ok = 0;
		LOG_DBG("sql fail: %s . err=%s\n", cmdbuff, sqlerr );
		goto close_db;
	}

	max_row = data->datalen/op->size;
	data->datalen = 0;
    for (i = 0; i < nRow && i < max_row; i++)
    {
        op->decode((void *)(data->data + data->datalen), &dbResult[(i+1) * nColumn]);
        data->datalen += (i+1)*op->size;
    }
	
	sqlite3_free_table(dbResult);	
close_db:
	if (sqlerr)
		sqlite3_free(sqlerr);

	sqlite3_close(db);
	return ok;
}





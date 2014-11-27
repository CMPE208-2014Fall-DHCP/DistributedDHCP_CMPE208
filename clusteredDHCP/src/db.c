
#include "config.h"


void init_db(dhcp_config *conf)
{
	
}

void sqlite_read(db_data *data)
{

}
int db_getleasetime()
{

	return 600;
}
int db_getrouter()
{
	return inet_addr("192.168.100.130");	

}
int db_getnetmask()
{
	return inet_addr("255.255.255.0");	

}

int db_gethostname()
{
//insert hostname into db when init 
}

void sqlite_write(db_data *data)
{
	sqlite3 *db = NULL;
	char *sqlerr = NULL;
	char cmdbuff[256];
	int ret;
    db_op *op;

	if(DATA_BUTT == data->type) {
		LOG_ERR("get unexpected data type!\n");
		return;
	}

	op = get_dbop(data->type);
	if(op == NULL) {
		LOG_ERR("not installed for type %d!\n", data->type);
		return;
	}
	
	ret = sqlite3_open(op->db_filename, &db);
	if (ret != 0 || db == NULL){
        LOG_ERR("open %s fail.\n", op->db_filename);
		return;
	}

	if (sqlite3_busy_timeout(db, 20) != 0)
		goto close_db;

	ret = sqlite3_exec(db, op->table_create,NULL, NULL, &sqlerr);
	if (ret != 0) {
		LOG_ERR("alter table fail! %s\n", sqlerr );
		goto close_db;
	}

	op->func(cmdbuff);
	#if 0
	snprintf(cmdbuff, sizeof(cmdbuff),
			   "INSERT INTO SCAN_RESULT values ( %d, %d, %d, %d, \"%s\", \"%s\",\"%s\")",
			   transid, src, dst, seq, router_name, in_port, out_port);
    #endif

	if (sqlite3_exec(db, (const char *)cmdbuff, NULL, NULL, &sqlerr) != 0)
		LOG_ERR("insert into table SCAN_RESULT fail! %s\n", sqlerr );

close_db:
	if (sqlerr)
		sqlite3_free(sqlerr);

	sqlite3_close(db);
	return;
}




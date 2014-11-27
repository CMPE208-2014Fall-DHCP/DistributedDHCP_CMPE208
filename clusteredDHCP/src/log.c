
#include "config.h"

FILE	*logfile;
extern dhcp_config  dhcpconf;


void init_log(dhcp_config *conf)
{
    if(!strlen(conf->log_filename))
		strcpy(conf->log_filename, DEFAULT_DHCP_LOG);
	
	logfile = fopen(conf->log_filename, "at");
	if(logfile == NULL){
		printf("open log file %s fail, output debug info on screen\n");
		conf->display = 1;
		conf->loglevel = LOG_BUTT;
	}
}

void exit_log()
{
    if(!logfile)
		return;
	
    fflush(logfile);
	fclose(logfile);
}

void log(int loglevel, int b, char *format, ...)
{
		va_list args;
		char buf[256];
	
		va_start(args, format);
		vsnprintf(buf, sizeof(buf), format, args);
		va_end(args);

	    if(dhcpconf.display)
			printf("%s\n", buf);

        //output to file
	    if(dhcpconf.loglevel <= loglevel) {
             fprintf(logfile, "[%s] %s\n", buffer, mess);
             fflush(logfile);
    	}
}



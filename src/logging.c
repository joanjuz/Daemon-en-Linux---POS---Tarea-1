#include "logging.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <syslog.h>

static FILE *g_log = NULL;

void log_open(const char *path){
    g_log = fopen(path,"a");
    if(!g_log){ openlog("imageserver", LOG_PID|LOG_CONS, LOG_DAEMON); }
}

void log_msg(const char *lvl, const char *fmt, ...){
    va_list ap; va_start(ap, fmt);
    char ts[64]; time_t t=time(NULL); struct tm tmv; localtime_r(&t,&tmv);
    strftime(ts,sizeof ts,"%Y-%m-%d %H:%M:%S",&tmv);
    if(g_log){
        fprintf(g_log,"[%s] [%s] ", ts, lvl);
        vfprintf(g_log, fmt, ap);
        fputc('\n', g_log);
        fflush(g_log);
    }else{
        char buf[1024]; vsnprintf(buf,sizeof buf,fmt,ap);
        syslog(LOG_INFO, "%s", buf);
    }
    va_end(ap);
}

void log_close(void){
    if(g_log) fclose(g_log);
    g_log=NULL;
    closelog();
}

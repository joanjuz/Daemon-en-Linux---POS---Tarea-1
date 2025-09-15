#include "config.h"
#include "logging.h"
#include "jobs.h"
#include "net.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>


#define PIDFILE "/run/imageserver.pid"

static volatile sig_atomic_t g_run = 1;

static void on_sig(int s){
    (void)s; g_run=0; net_stop();
}

static void daemonize(void){
    pid_t pid=fork(); if(pid<0) exit(1); if(pid>0) exit(0);
    if(setsid()<0) exit(1);
    pid=fork(); if(pid<0) exit(1); if(pid>0) exit(0);
    umask(027); chdir("/");
    int fd=open("/dev/null",O_RDWR); if(fd>=0){ dup2(fd,0); dup2(fd,1); dup2(fd,2); if(fd>2) close(fd); }
}

static void write_pid(void){ FILE*f=fopen(PIDFILE,"w"); if(f){ fprintf(f,"%d\n", getpid()); fclose(f);} }
static pid_t read_pid(void){ FILE*f=fopen(PIDFILE,"r"); if(!f) return -1; pid_t p=-1; if(fscanf(f,"%d",&p)!=1) p=-1; fclose(f); return p; }

static int cmd_start(void){
    pid_t ex = read_pid(); if(ex>0 && kill(ex,0)==0){ fprintf(stderr,"imageserver ya en ejecucion (pid=%d)\n", ex); return 0; }
    daemonize(); write_pid();
    cfg_load("/etc/server/config.conf");
    log_open(cfg()->log_file);
    log_msg("INFO","Iniciando en puerto %d", cfg()->port);

    // preparar workers + red
    if(jobs_init(cfg()->workers)<0){ log_msg("ERR","jobs_init"); return 1; }
    if(net_start(cfg()->port)<0){ log_msg("ERR","net_start"); return 1; }

    signal(SIGTERM,on_sig); signal(SIGINT,on_sig); signal(SIGHUP,on_sig);

    while(g_run) pause();

    // shutdown
    jobs_shutdown();
    log_msg("INFO","Detenido");
    log_close();
    unlink(PIDFILE);
    return 0;
}

static int cmd_stop(void){
    pid_t p = read_pid(); if(p<=0){ fprintf(stderr,"No hay PID activo\n"); return 1; }
    if(kill(p, SIGTERM)==-1){ perror("kill"); return 1; }
    for(int i=0;i<50;i++){ if(kill(p,0)==-1) break; usleep(100*1000); }
    unlink(PIDFILE);
    printf("Detenido\n");
    return 0;
}
static int cmd_status(void){
    pid_t p=read_pid(); if(p>0 && kill(p,0)==0){ printf("imageserver activo (pid=%d)\n", p); return 0; }
    printf("imageserver NO esta activo\n"); return 3;
}
static int cmd_restart(void){ cmd_stop(); sleep(1); return cmd_start(); }

int main(int argc,char**argv){
    if(argc<2){ fprintf(stderr,"Uso: %s {start|stop|status|restart}\n", argv[0]); return 1; }
    if(!strcmp(argv[1],"start"))   return cmd_start();
    if(!strcmp(argv[1],"stop"))    return cmd_stop();
    if(!strcmp(argv[1],"status"))  return cmd_status();
    if(!strcmp(argv[1],"restart")) return cmd_restart();
    fprintf(stderr,"Comando no valido\n"); return 1;
}

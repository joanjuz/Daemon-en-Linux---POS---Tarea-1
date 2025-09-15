#include "net.h"
#include "config.h"
#include "logging.h"
#include "jobs.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>      // open, O_CREAT, O_WRONLY, O_TRUNC
#include <sys/stat.h>   // mode_t, permisos 0644
#include <unistd.h>     // close, write
#include <string.h>     // strdup, memset, strncpy


static int listen_fd = -1;
static pthread_t th_accept;
static int running = 0;

static uint64_t ntohll_u64(uint64_t v){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return (((uint64_t)ntohl((uint32_t)(v & 0xffffffff)))<<32) | ntohl((uint32_t)(v>>32));
#else
    return v;
#endif
}

static ssize_t r_full(int fd, void *buf, size_t len){
    size_t off=0; while(off<len){ ssize_t r=recv(fd,(char*)buf+off,len-off,0); if(r==0) return off; if(r<0){ if(errno==EINTR) continue; return -1;} off+=r; } return (ssize_t)off;
}
static ssize_t w_full(int fd, const void *buf, size_t len){
    size_t off=0; while(off<len){ ssize_t w=send(fd,(char*)buf+off,len-off,0); if(w<=0){ if(errno==EINTR) continue; return -1;} off+=w; } return (ssize_t)off;
}

static void handle_client(int cfd, const char *ip){
    uint32_t nlen_n; uint64_t fsz_n;
    if(r_full(cfd,&nlen_n,sizeof nlen_n)!=(ssize_t)sizeof nlen_n) return;
    if(r_full(cfd,&fsz_n, sizeof fsz_n)!=(ssize_t)sizeof fsz_n) return;
    uint32_t nlen = ntohl(nlen_n);
    uint64_t fsz  = ntohll_u64(fsz_n);
    if(!nlen || nlen>1024 || !fsz) return;

    char *name = (char*)malloc(nlen+1); if(!name) return;
    if(r_full(cfd,name,nlen)!=(ssize_t)nlen){ free(name); return; }
    name[nlen]='\0';

    const char *tmp = cfg()->tmp_dir;
    char *tpath=NULL; asprintf(&tpath, "%s/%ld_%s", tmp, (long)time(NULL), name);
    int out = open(tpath, O_CREAT|O_WRONLY|O_TRUNC, 0644);
    if(out<0){ log_msg("ERR","open tmp: %s", strerror(errno)); free(name); free(tpath); return; }

    char buf[8192]; uint64_t rem=fsz;
    while(rem){
        size_t want = rem>sizeof buf? sizeof buf: (size_t)rem;
        ssize_t r = recv(cfd, buf, want, 0);
        if(r<=0){ if(errno==EINTR) continue; break; }
        if(write(out, buf, (size_t)r)<0){ log_msg("ERR","write tmp"); break; }
        rem -= (size_t)r;
    }
    close(out);

    if(rem==0){
        Job *j = (Job*)calloc(1,sizeof *j);
        j->tmp_path = tpath; j->orig_name=strdup(name); j->size_bytes=(size_t)fsz;
        strncpy(j->client_ip, ip, sizeof j->client_ip - 1);
        jobs_push(j);
        w_full(cfd, "OK", 2);
    } else {
        unlink(tpath); free(tpath);
    }
    free(name);
}

static void* accept_loop(void*arg){
    (void)arg;
    struct sockaddr_in cli; socklen_t cl=sizeof cli;
    while(running){
        int cfd = accept(listen_fd, (struct sockaddr*)&cli, &cl);
        if(cfd<0){ if(errno==EINTR) continue; log_msg("ERR","accept: %s", strerror(errno)); continue; }
        char ip[64]; inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof ip);
        handle_client(cfd, ip);
        close(cfd);
    }
    return NULL;
}

int net_start(int port){
    listen_fd = socket(AF_INET,SOCK_STREAM,0);
    if(listen_fd<0) return -1;
    int opt=1; setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof opt);
    struct sockaddr_in a={0}; a.sin_family=AF_INET; a.sin_port=htons(port); a.sin_addr.s_addr=htonl(INADDR_ANY);
    if(bind(listen_fd,(struct sockaddr*)&a,sizeof a)<0) return -1;
    if(listen(listen_fd,64)<0) return -1;
    running=1;
    pthread_create(&th_accept,NULL,accept_loop,NULL);
    return 0;
}

void net_stop(void){
    running=0;
    if(listen_fd!=-1){ close(listen_fd); listen_fd=-1; }
}

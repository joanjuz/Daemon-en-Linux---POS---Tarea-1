// client.c - Cliente para ImageServer
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

static uint64_t htonll_u64(uint64_t v){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return (((uint64_t)htonl((uint32_t)(v & 0xffffffff)))<<32) | htonl((uint32_t)(v>>32));
#else
    return v;
#endif
}

int send_file(const char *ip, int port, const char *path){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock<0){ perror("socket"); return -1; }

    struct sockaddr_in serv;
    memset(&serv,0,sizeof serv);
    serv.sin_family=AF_INET;
    serv.sin_port=htons(port);
    if(inet_pton(AF_INET, ip, &serv.sin_addr)<=0){
        perror("inet_pton"); close(sock); return -1;
    }
    if(connect(sock,(struct sockaddr*)&serv,sizeof serv)<0){
        perror("connect"); close(sock); return -1;
    }

    // obtener tamaño del archivo
    struct stat st;
    if(stat(path,&st)<0){ perror("stat"); close(sock); return -1; }
    uint64_t fsz = (uint64_t)st.st_size;

    // nombre de archivo
    const char *fname = strrchr(path,'/');
    fname = fname? fname+1 : path;
    uint32_t nlen = (uint32_t)strlen(fname);

    // enviar encabezado
    uint32_t nlen_n = htonl(nlen);
    uint64_t fsz_n  = htonll_u64(fsz);
    if(send(sock,&nlen_n,sizeof nlen_n,0)!=sizeof nlen_n){ perror("send nlen"); close(sock); return -1; }
    if(send(sock,&fsz_n,sizeof fsz_n,0)!=sizeof fsz_n){ perror("send fsz"); close(sock); return -1; }
    if(send(sock,fname,nlen,0)!=(ssize_t)nlen){ perror("send name"); close(sock); return -1; }

    // enviar archivo en bloques
    int fd = open(path,O_RDONLY);
    if(fd<0){ perror("open file"); close(sock); return -1; }
    char buf[8192]; ssize_t r;
    while((r=read(fd,buf,sizeof buf))>0){
        ssize_t w=send(sock,buf,r,0);
        if(w!=r){ perror("send data"); close(fd); close(sock); return -1; }
    }
    close(fd);

    // esperar respuesta
    char resp[16]={0};
    ssize_t rr=recv(sock,resp,sizeof resp-1,0);
    if(rr>0) printf("Respuesta servidor: %s\n", resp);

    close(sock);
    return 0;
}

int main(int argc,char**argv){
    if(argc<3){
        fprintf(stderr,"Uso: %s <ip_servidor> <puerto>\n", argv[0]);
        return 1;
    }
    const char* ip=argv[1];
    int port=atoi(argv[2]);

    char path[1024];
    while(1){
        printf("Ingrese ruta de imagen (o Exit para salir): ");
        if(!fgets(path,sizeof path,stdin)) break;
        path[strcspn(path,"\n")]=0;
        if(!strcmp(path,"Exit")) break;
        if(path[0]) send_file(ip,port,path);
    }
    return 0;
}

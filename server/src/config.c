#include "config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static ServerConfig G;

static void set_defaults(void){
    G.port = 1717;
    snprintf(G.output_dir, sizeof G.output_dir, "%s", "/var/lib/imageserver");
    snprintf(G.log_file,   sizeof G.log_file,   "%s", "/var/log/imageserver.log");
    snprintf(G.tmp_dir,    sizeof G.tmp_dir,    "%s", "/var/lib/imageserver/tmp");
    snprintf(G.dir_red,    sizeof G.dir_red,    "%s", "rojas");
    snprintf(G.dir_green,  sizeof G.dir_green,  "%s", "verdes");
    snprintf(G.dir_blue,   sizeof G.dir_blue,   "%s", "azules");
    snprintf(G.dir_eq,     sizeof G.dir_eq,     "%s", "equalizadas");
    G.workers = 2;
}

static void trim(char *s){
    size_t n=strlen(s);
    while(n && (s[n-1]=='\n'||s[n-1]=='\r'||s[n-1]==' '||s[n-1]=='\t')) s[--n]='\0';
}

void cfg_load(const char *path){
    set_defaults();
    FILE *f=fopen(path,"r");
    if(!f) return; // defaults
    char line[1024];
    while(fgets(line,sizeof line,f)){
        trim(line);
        if(!line[0] || line[0]=='#') continue;
        char *eq=strchr(line,'='); if(!eq) continue; *eq='\0';
        char *k=line, *v=eq+1;
        if(!strcmp(k,"PORT")) G.port = atoi(v);
        else if(!strcmp(k,"OUTPUT_DIR")) snprintf(G.output_dir,sizeof G.output_dir,"%s",v);
        else if(!strcmp(k,"LOG_FILE")) snprintf(G.log_file,sizeof G.log_file,"%s",v);
        else if(!strcmp(k,"WORKERS")) G.workers = atoi(v);
        else if(!strcmp(k,"DIR_RED")) snprintf(G.dir_red,sizeof G.dir_red,"%s",v);
        else if(!strcmp(k,"DIR_GREEN")) snprintf(G.dir_green,sizeof G.dir_green,"%s",v);
        else if(!strcmp(k,"DIR_BLUE")) snprintf(G.dir_blue,sizeof G.dir_blue,"%s",v);
        else if(!strcmp(k,"DIR_EQUALIZED")) snprintf(G.dir_eq,sizeof G.dir_eq,"%s",v);
        else if(!strcmp(k,"TMP_DIR")) snprintf(G.tmp_dir,sizeof G.tmp_dir,"%s",v);
    }
    fclose(f);
}
const ServerConfig* cfg(){ return &G; }

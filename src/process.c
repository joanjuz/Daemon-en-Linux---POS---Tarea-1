#include "process.h"
#include "config.h"
#include "logging.h"
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Implementaciones STB solo aquí
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static void ensure_dir(const char *path){
    struct stat st;
    if(stat(path,&st)==-1){ mkdir(path,0755); }
}
static char* path2(const char*a,const char*b){ char*res=NULL; asprintf(&res,"%s/%s",a,b); return res; }

static void equalize_rgb(unsigned char *data,int w,int h,int ch){
    if(ch<3) return;
    const int N=w*h;
    for(int c=0;c<3;c++){
        unsigned int hist[256]={0}, cdf[256], acc=0;
        for(int i=0;i<N;i++) hist[data[i*ch+c]]++;
        for(int v=0; v<256; v++){ acc+=hist[v]; cdf[v]=acc; }
        if(!acc) continue;
        for(int i=0;i<N;i++){
            unsigned char px = data[i*ch+c];
            data[i*ch+c] = (unsigned char)((cdf[px]*255.0)/(double)N);
        }
    }
}
static const char* dominant(unsigned char*data,int w,int h,int ch){
    unsigned long long r=0,g=0,b=0; int N=w*h;
    for(int i=0;i<N;i++){ r+=data[i*ch+0]; g+=data[i*ch+1]; b+=data[i*ch+2]; }
    if (r >= g && r >= b) return "R";
    if (g >= r && g >= b) return "G";
    return "B";
}

int process_job(const Job *j){
    const ServerConfig *C = cfg();
    int w,h,ch; unsigned char*img = stbi_load(j->tmp_path,&w,&h,&ch,0);
    if(!img){ log_msg("ERR","stbi_load fallo '%s'", j->tmp_path); return -1; }

    // equalize
    equalize_rgb(img,w,h,ch);

    // dirs
    ensure_dir(C->output_dir);
    ensure_dir(C->tmp_dir);
    char *eq_dir = path2(C->output_dir, C->dir_eq); ensure_dir(eq_dir);

    // save equalized (PNG)
    char *eq_path = path2(eq_dir, j->orig_name);
    int ok=0;
    if(ch==3) ok = stbi_write_png(eq_path, w,h,3,img,w*3);
    else if(ch==4) ok = stbi_write_png(eq_path, w,h,4,img,w*4);
    else{
        unsigned char *rgb = (unsigned char*)malloc((size_t)w*h*3);
        if(!rgb){ log_msg("ERR","malloc rgb"); stbi_image_free(img); free(eq_dir); free(eq_path); return -1; }
        for(int i=0;i<w*h;i++){
            unsigned char R=img[i*ch+0], G=(ch>1?img[i*ch+1]:R), B=(ch>2?img[i*ch+2]:R);
            rgb[i*3+0]=R; rgb[i*3+1]=G; rgb[i*3+2]=B;
        }
        ok = stbi_write_png(eq_path, w,h,3,rgb,w*3);
        free(rgb);
    }
    if(!ok){ log_msg("ERR","No se pudo escribir '%s'", eq_path); stbi_image_free(img); free(eq_dir); free(eq_path); return -1; }

    // classify
    const char *dom = dominant(img,w,h,ch);
    const char *dname = (!strcmp(dom,"R")? C->dir_red : (!strcmp(dom,"G")? C->dir_green : C->dir_blue));
    char *cdir = path2(C->output_dir, dname); ensure_dir(cdir);
    char *cpath = path2(cdir, j->orig_name);

    // copy eq_path -> cpath
    FILE *src=fopen(eq_path,"rb"), *dst=fopen(cpath,"wb");
    if(src && dst){
        char buf[8192]; size_t n;
        while((n=fread(buf,1,sizeof buf,src))>0) fwrite(buf,1,n,dst);
    }
    if(src) fclose(src);
    if(dst) fclose(dst);

    log_msg("INFO","cliente=%s archivo=%s tam=%zu eq='%s' clasif='%s'",
            j->client_ip, j->orig_name, j->size_bytes, eq_path, cpath);

    stbi_image_free(img);
    unlink(j->tmp_path);
    free(eq_dir); free(eq_path); free(cdir); free(cpath);
    return 0;
}

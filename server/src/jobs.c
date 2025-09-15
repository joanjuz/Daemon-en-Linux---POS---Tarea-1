#include "jobs.h"
#include "logging.h"
#include "process.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_JOBS 4096
static Job *heap[MAX_JOBS];
static size_t heap_sz=0;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cv  = PTHREAD_COND_INITIALIZER;
static int running = 0;
static int W = 0;
static pthread_t *ths = NULL;

static void swap(size_t i,size_t j){ Job*t=heap[i]; heap[i]=heap[j]; heap[j]=t; }
static void up(size_t i){ while(i){ size_t p=(i-1)/2; if(heap[p]->size_bytes<=heap[i]->size_bytes) break; swap(i,p); i=p; } }
static void down(size_t i){
    while(1){
        size_t l=2*i+1,r=2*i+2,m=i;
        if(l<heap_sz && heap[l]->size_bytes<heap[m]->size_bytes) m=l;
        if(r<heap_sz && heap[r]->size_bytes<heap[m]->size_bytes) m=r;
        if (m == i) break;
        swap(i, m);
        i = m;
    }
}

void jobs_push(Job *j){
    pthread_mutex_lock(&mtx);
    if(heap_sz<MAX_JOBS){ heap[heap_sz]=j; up(heap_sz); heap_sz++; pthread_cond_signal(&cv); }
    else { log_msg("ERR","Cola de trabajos llena"); }
    pthread_mutex_unlock(&mtx);
}

static Job* pop(void){
    pthread_mutex_lock(&mtx);
    while(running && heap_sz==0) pthread_cond_wait(&cv,&mtx);
    Job *j=NULL;
    if(heap_sz){ j=heap[0]; heap[0]=heap[--heap_sz]; down(0); }
    pthread_mutex_unlock(&mtx);
    return j;
}

void job_free(Job *j){ if(!j) return; free(j->tmp_path); free(j->orig_name); free(j); }

static void* worker(void*arg){
    (void)arg;
    while(running){
        Job *j = pop();
        if(!j) continue;
        process_job(j);
        job_free(j);
    }
    return NULL;
}

int jobs_init(int workers){
    running=1; W=workers>0?workers:1; if(W>32) W=32;
    ths = (pthread_t*)calloc((size_t)W, sizeof *ths);
    if(!ths) return -1;
    for(int i=0;i<W;i++) pthread_create(&ths[i],NULL,worker,NULL);
    return 0;
}

void jobs_shutdown(void){
    pthread_mutex_lock(&mtx); running=0; pthread_cond_broadcast(&cv); pthread_mutex_unlock(&mtx);
    for(int i=0;i<W;i++) pthread_join(ths[i],NULL);
    free(ths); ths=NULL; W=0;
    // limpiar trabajos pendientes
    while(heap_sz){ job_free(heap[--heap_sz]); }
}

#pragma once
#include <stddef.h>

typedef struct Job {
    char *tmp_path;
    char *orig_name;
    size_t size_bytes;
    char client_ip[64];
} Job;

int  jobs_init(int workers);
void jobs_shutdown(void);
void jobs_push(Job *j);
void job_free(Job *j);

#pragma once
#include <stddef.h>

typedef struct {
    int   port;
    char  output_dir[512];
    char  log_file[512];
    char  tmp_dir[512];
    char  dir_red[128];
    char  dir_green[128];
    char  dir_blue[128];
    char  dir_eq[128];
    int   workers;
} ServerConfig;

void cfg_load(const char *path);           // lee conf o usa defaults
const ServerConfig* cfg();                 // acceso de solo lectura

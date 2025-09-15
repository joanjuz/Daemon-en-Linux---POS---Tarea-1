#pragma once
int  net_start(int port);        // crea socket y lanza hilo de aceptación
void net_stop(void);             // cierra socket y termina hilo

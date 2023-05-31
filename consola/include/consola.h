#ifndef CONSOLA_H_
#define CONSOLA_H_

#include <protocolo.h>
#include <sockets.h>
#include <shared.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <string.h>
#include <pthread.h>
#define MAX_LINE_LENGTH 256

// Variables globales
t_log* logger;
t_config* config;

// Variables config
char *IP_KERNEL;
char *PUERTO_KERNEL;

int fd_kernel;

void leer_config();
t_list* leer_instrucciones(char* path, t_log* logger);
static void procesar_conexion();

#endif /* CONSOLA_H_ */

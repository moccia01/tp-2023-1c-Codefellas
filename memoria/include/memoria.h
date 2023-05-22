#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <sockets.h>
#include <protocolo.h>
#include <commons/log.h>
#include <commons/config.h>
#include <shared.h>
#include <pthread.h>

// Variables globales
t_log* logger;
t_config* config;

int fd_memoria, fd_filesystem, fd_cpu, fd_kernel;
char* server_name;

// Variables del config
char* IP;
char* PUERTO;

// --------------------- INIT ---------------------
void leer_config();

// --------------------- COMUNICACION ---------------------
static void procesar_conexion(void *void_args);
void iterator(char *value);
int server_escuchar(int server_socket);

#endif /* MEMORIA_H_ */

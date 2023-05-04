#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <commons/log.h>
#include <commons/config.h>
#include <sockets.h>
#include <protocolo.h>
#include <shared.h>
#include <pthread.h>

// Variables globales
t_log* logger;
t_log* logger_obligatorio;
t_config* config;

// Variables del config
char* IP;
char* PUERTO;
char* IP_MEMORIA;
char* PUERTO_MEMORIA;

// Variables de conexion
int fd_filesystem;
int socket_cliente;
int fd_memoria;
char* server_name;

void leer_config();
void inicializar_variables();

// COMUNICACION
static void procesar_conexion();
void server_escuchar();


#endif /* FILESYSTEM_H_ */

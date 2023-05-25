#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <sockets.h>
#include <protocolo.h>
#include <commons/log.h>
#include <commons/config.h>
#include <shared.h>
#include <pthread.h>



typedef enum {
	FIRST_FIT,
	BEST_FIT,
	WORST_FIT
} t_algoritmo_memoria;

//Config
char* IP;
char* PUERTO;
char* PUERTO_ESCUCHA;
char* TAM_MEMORIA;
char* TAM_SEGMENTO_0;
char* CANT_SEGMENTOS;
char* RETARDO_MEMORIA;
char* RETARDO_COMPACTACION;
t_algoritmo_memoria ALGORITMO_ASIGNACION;

// Variables globales
t_log* logger;
t_config* config;

int fd_memoria, fd_filesystem, fd_cpu, fd_kernel;
char* server_name;


// --------------------- INIT ---------------------
void leer_config();
void asignar_algoritmo_memoria(char *algoritmo_memoria);
// --------------------- COMUNICACION ---------------------
static void procesar_conexion(void *void_args);
void iterator(char *value);
int server_escuchar(int server_socket);

//UTILS
char* IP;
char* PUERTO;
char* PUERTO_ESCUCHA;
char* TAM_MEMORIA;
char* TAM_SEGMENTO_0;
char* CANT_SEGMENTOS;
char* RETARDO_MEMORIA;
char* RETARDO_COMPACTACION;
t_algoritmo_memoria ALGORITMO_ASIGNACION;

#endif /* MEMORIA_H_ */

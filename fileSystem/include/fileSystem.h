#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <commons/log.h>
#include <commons/config.h>
#include <sockets.h>
#include <protocolo.h>
#include <shared.h>
#include <pthread.h>
#include <commons/bitarray.h>

// Variables globales
t_log* logger;
t_log* logger_obligatorio;
t_config* config;
FILE *superbloque;
FILE *bitmap;
FILE *bloques;

// Variables del config
char* IP;
char* PUERTO;
char* IP_MEMORIA;
char* PUERTO_MEMORIA;
char* PATH_SUPERBLOQUE;
char* PATH_BITMAP;
char* PATH_BLOQUES;
char* PATH_FCB;
char* RETARDO_ACCESO_BLOQUE;

// Variables de conexion
int fd_filesystem;
int socket_cliente;
int fd_memoria;
char* server_name;

void leer_config();
void inicializar_variables();
void levantar_archivos();
void terminar_programa();

// COMUNICACION
static void procesar_conexion();
void server_escuchar();

// Estructuras
/*
typedef struct{
	int block_size;
	int block_count;
}superbloque;

*/

typedef struct{
	char* nombre_archivo;
	int tamanio_archivo;
	char* puntero_directo;
	char* puntero_indirecto;	//Chequear tipos de datos
}fcb;

#endif /* FILESYSTEM_H_ */

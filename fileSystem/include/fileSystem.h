#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

//#include <fcntl.h> //Esta libreria es para la funcion open
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <sockets.h>
#include <protocolo.h>
#include <shared.h>
#include <pthread.h>
#include <commons/bitarray.h>
#include <sys/mman.h>

// Variables globales
t_log* logger;
t_log* logger_obligatorio;
t_config* config;
t_config* superbloque;
t_bitarray* bitmap;
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

//Variables del superbloque
int BLOCK_SIZE;
int BLOCK_COUNT;

//Variable del bitmap
int* ARRAY_BITMAP;

//Variable de archivo de bloques
int TAMANIO;
char* ARRAY_BLOQUES;
/*
typedef enum{
	PROT_READ,
	PROT_WRITE,
	MAP_SHARED,
	MAP_FAILED,
} t_modos_mmap;
*/


void leer_config();
void levantar_archivos();
void terminar_programa();
void inicializar_variables();

// ARCHIVOS
void leer_superbloque();
void crear_bitmap();
void crear_archivo_de_bloques();

// COMUNICACION
static void procesar_conexion();
void server_escuchar();

// Estructuras
typedef struct{
	char* nombre_archivo;
	int tamanio_archivo;
	uint32_t puntero_directo;
	uint32_t puntero_indirecto;
}fcb;

typedef struct{

}t_bloque;

#endif /* FILESYSTEM_H_ */

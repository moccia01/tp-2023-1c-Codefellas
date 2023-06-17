#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <fcntl.h> //Esta libreria es para la funcion open
#include <sys/stat.h>
#include <math.h>
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
t_bitarray* bitmap; //Esta nombrado asi xq basicamente el bitmap tiene el bitarray y nada mas
FILE *bloques;
t_list* lista_fcbs;

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

// Variables del superbloque
int BLOCK_SIZE;
int BLOCK_COUNT;

// Variable del bitmap
int* ARRAY_BITMAP;

// Variable de archivo de bloques
int TAMANIO;
char* ARRAY_BLOQUES;
void* buffer_bitmap;
void* buffer_bloques;

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

// Operaciones
bool existe_fcb(char* nombre_archivo);
t_config* obtener_archivo(char* nombre_archivo);
void manejar_f_open(char* nombre_archivo);
void manejar_f_create(char* nombre_archivo);
void manejar_f_truncate(char* nombre_archivo, int tamanio);
void manejar_f_read(char* nombre_archivo, int dir_fisica, int tamanio);
void manejar_f_write(char* nombre_archivo, int dir_fisica, int tamanio);

// Estructuras
typedef struct{
	char* nombre_archivo;
	int tamanio_archivo;
	uint32_t puntero_directo;
	uint32_t puntero_indirecto;
}fcb;

typedef struct{
	char* contenido;
	uint32_t* punteros;
}t_bloque;

typedef struct{
	char* nombre_archivo;
	t_config* archivo_fcb;
}t_archivo;

#endif /* FILESYSTEM_H_ */

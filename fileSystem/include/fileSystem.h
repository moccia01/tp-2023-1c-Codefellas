#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

//#include <bits/mman-linux.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/log.h>
#include <dirent.h>		//Esta libreria es para readdir
#include <fcntl.h> 		//Esta libreria es para open
#include <math.h>
#include <protocolo.h>
#include <pthread.h>
#include <shared.h>
#include <sockets.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>

// Variables globales
t_log* logger;
t_log* logger_obligatorio;
t_config* config;
t_config* superbloque;
t_bitarray* bitmap; //Esta nombrado asi xq basicamente el bitmap tiene el bitarray y nada mas
FILE *bloques;
t_list* lista_fcbs;
t_list* peticiones_pendientes;
sem_t contador_peticiones;
pthread_mutex_t mutex_peticiones_pendientes;

typedef enum{
	OPEN,
	TRUNCATE,
	READ,
	WRITE,
}t_operacion_fs;

typedef struct{
	t_operacion_fs operacion;
	char* nombre;
	int tamanio;
	int dir_fisica;
}t_peticion;

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
int fd_memoria;
int socket_cliente;
char* server_name;

// Variables del superbloque
int BLOCK_SIZE;
int BLOCK_COUNT;

// Variable del bitmap
int tamanio_bitmap;

// Variable de archivo de bloques
int TAMANIO_ARCHIVO_BLOQUES;
//char* ARRAY_BLOQUES;
void* buffer_bitmap;
void* buffer_bloques;

// Funciones
void leer_config();
void levantar_archivos();
void terminar_programa();
void inicializar_variables();
void inicializar_fcbs();
void leer_superbloque();
void crear_bitmap();
void crear_archivo_de_bloques();
void iniciar_atencion_peticiones();
void atender_peticiones();
void agrego_a_pendientes(t_peticion* peticion);
t_peticion* saco_de_pendientes();

// COMUNICACION
static void procesar_conexion();
void server_escuchar();

// Operaciones
t_peticion* crear_peticion(t_operacion_fs operacion, char* nombre, int tamanio, int dir_fisica);
void manejar_peticion(t_peticion* peticion);
bool existe_fcb(char* nombre_archivo);
t_config* obtener_archivo(char* nombre_archivo);
void manejar_f_open(char* nombre_archivo);
void manejar_f_create(char* nombre_archivo);
void manejar_f_truncate(char* nombre_archivo, int tamanio);
void manejar_f_read(char* nombre_archivo, int dir_fisica, int tamanio);
void manejar_f_write(char* nombre_archivo, int dir_fisica, int tamanio);

//Funciones auxiliares de f_truncate
void asignar_bloques(int cant_bloques, t_config* archivo);
uint32_t buscar_bloque_libre();

// Estructuras
typedef struct{
	char* nombre_archivo;
	int tamanio_archivo;
	uint32_t puntero_directo;
	uint32_t puntero_indirecto;
}t_fcb;

typedef struct{
	char* nombre_archivo;
	t_config* archivo_fcb;
}t_archivo;

#endif /* FILESYSTEM_H_ */

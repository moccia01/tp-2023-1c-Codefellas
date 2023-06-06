
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

typedef struct {
	int base;
	int tamanio;
}t_hueco_libre;

//Config
char* IP;
char* PUERTO;
char* PUERTO_ESCUCHA;
int TAM_MEMORIA;
int TAM_SEGMENTO_0;
int CANT_SEGMENTOS;
int RETARDO_MEMORIA;
int RETARDO_COMPACTACION;
t_algoritmo_memoria ALGORITMO_ASIGNACION;

// Variables globales
t_log* logger;
t_config* config;
int fd_memoria, fd_filesystem, fd_cpu, fd_kernel;
char* server_name;
void *espacio_memoria;
t_segmento* segmento_0;

// esto lo hice yo (tomy) para la actualizacion de tablas de segmentos post compactacion
t_list* lista_ts_wrappers;
t_list* huecos_libres;

// --------------------- INIT ---------------------
void leer_config();
void asignar_algoritmo_memoria(char *algoritmo_memoria);
void terminar_programa();
void inicializar_variables();

// --------------------- COMUNICACION ---------------------
static void procesar_conexion(void *void_args);
void iterator(char *value);
int server_escuchar(int server_socket);

t_segment_response verificar_espacio_memoria(int tamanio);
t_list* inicializar_tabla_segmento(int id_proceso);
void inicializar_memoria();
void terminar_proceso(int pid);

#endif /* MEMORIA_H_ */
// aparece y segmento_0
// cada segmento va a tener base y tamaño

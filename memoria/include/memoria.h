
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
}t_hueco_memoria;

typedef struct{
	int pid;
	t_list* escrituras;
}t_escrituras;

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
int fd_memoria;
char* server_name;
void *espacio_usuario;
t_segmento* segmento_0;

// esto lo hice yo (tomy) para la actualizacion de tablas de segmentos post compactacion
t_list* lista_ts_wrappers;
t_list* huecos_libres;
t_list* huecos_escritos;

// --------------------- INIT ---------------------
void leer_config();
void asignar_algoritmo_memoria(char *algoritmo_memoria);
void terminar_programa();

// --------------------- COMUNICACION ---------------------
static void procesar_conexion(void *void_args);
void iterator(char *value);
int server_escuchar(int server_socket);

t_segment_response verificar_espacio_memoria(int tamanio);
t_list* inicializar_proceso(int id_proceso);
void inicializar_memoria();
void terminar_proceso(int pid);
void eliminar_escrituras_de_proceso(int pid);
void eliminar_tabla_segmentos(int pid);
int crear_segmento_segun_algoritmo(int id, int tamanio, int pid);
t_hueco_memoria* encontrar_hueco_first(int tamanio);
t_hueco_memoria* encontrar_hueco_best(int tamanio);
t_hueco_memoria* encontrar_hueco_worst(int tamanio);
t_segmento* crear_segmento(int pid, int id, int base, int tamanio);
void actualizar_hueco_libre(t_segmento* segmento_nuevo, t_hueco_memoria* hueco_viejo);
void actualizar_tabla_segmentos_de_proceso(int pid, t_segmento* segmento);
void deletear_segmento(int pid, int base);
void agregar_hueco_libre(int base, int tamanio);
#endif /* MEMORIA_H_ */
// aparece y segmento_0
// cada segmento va a tener base y tamaño

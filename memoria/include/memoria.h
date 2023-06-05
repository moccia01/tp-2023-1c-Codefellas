
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
    void* base;     // Dirección base del segmento
    int tamano;     // Tamaño del segmento en bytes
    //int permisos;   // Permisos de acceso (ejemplo: lectura, escritura)
} t_segmentoss;		//TODO: Ponerle otro nombre ya que t_segmento está usado en las shared


typedef struct {
    int capacidad;      // Capacidad de la tabla
    int numSegmentos;	// Número actual de segmentos en la tabla = 0 NO SE PUEDE INICIALIZAR ACA
    int id;
    t_list* lista_segmentos;
} t_tabla_segmento;

typedef struct {
	int base;
	int tamanio;
}t_hueco_libre;

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
t_list *lista_t_segmento;
t_list *lista;
pthread_mutex_t memoria_usuario_mutex;
pthread_mutex_t lista_de_tablas_de_segmentos_mutex;
int fd_memoria, fd_filesystem, fd_cpu, fd_kernel;
char* server_name;


// --------------------- INIT ---------------------
void leer_config();
void asignar_algoritmo_memoria(char *algoritmo_memoria);
void terminar_programa();
void *espacio_memoria;
void *segmentos_memoria;
void *segmento_0;
// --------------------- COMUNICACION ---------------------
static void procesar_conexion(void *void_args);
void iterator(char *value);
int server_escuchar(int server_socket);


t_segment_response verificar_espacio_memoria(int tamanio);
void inicializar_tabla_segmento(int id_proceso);
void inicializar_memoria();


#endif /* MEMORIA_H_ */
// aparece y segmento_0
// cada segmento va a tener base y tamaño

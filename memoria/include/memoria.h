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
t_log* logger_obligatorio;
t_config* config;
int fd_memoria;
int fd_kernel;
int fd_cpu;
int fd_filesystem;
char* server_name;
void *espacio_usuario;
t_segmento* segmento_0;
int tamanio_total;

// esto lo hice yo (tomy) para la actualizacion de tablas de segmentos post compactacion
t_list* lista_ts_wrappers; // lista de tablas de segmentos por proceso
t_list* huecos_libres; // lista de huecos libres para manejo segmentacion
t_list* segmentos_en_memoria;

// --------------------- INIT ---------------------
void leer_config();
void asignar_algoritmo_memoria(char *algoritmo_memoria);
void terminar_programa();

// --------------------- COMUNICACION ---------------------
static void procesar_conexion(void *void_args);
void iterator(char *value);
int server_escuchar();

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
t_list* deletear_segmento(int id_segmento, int pid);
void agregar_hueco_libre(int base, int tamanio);
void compactar();
void actualizar_segmento(int old_base, int new_base);
bool buscar_segmento_en_ts(int old_base, int new_base, t_list* tabla_segmentos);
void compactar_version_tomy();
bool comparador_de_base(t_segmento *, t_segmento *);
void log_resultado_compactacion();
void log_valor_espacio_usuario(char* valor, int tamanio);
void log_segmentos_en_memoria();

#endif /* MEMORIA_H_ */

#ifndef KERNEL_H_
#define KERNEL_H_

#include <protocolo.h>
#include <sockets.h>
#include <shared.h>
#include <commons/log.h>
#include <commons/config.h>
#include <pthread.h>
#include <semaphore.h>


typedef enum{
	FIFO,
	HRRN,
} t_algoritmo;

typedef struct{
	char* recurso;
	int id;
	int instancias;
	t_list* cola_block_asignada;
	pthread_mutex_t mutex_asignado;
}t_recurso;

typedef struct{
	char* nombre_archivo;
	int puntero;
	t_list* cola_block_asignada;
	pthread_mutex_t mutex_asignado;
}t_archivo;

typedef struct{
	int fd_consola;
	t_contexto_ejecucion* contexto_de_ejecucion;
	t_list* archivos_abiertos; // capaz esto necesita mutex (?
	uint16_t estimado_proxima_rafaga;
	time_t tiempo_ingreso_ready;
	time_t tiempo_ingreso_exec;
} t_pcb;

typedef struct{
	t_pcb* pcb;
	int tiempo;
}t_manejo_io;

t_log* logger;
t_log* logger_obligatorio;
t_config* config;
int server_socket;
int fd_cpu;
int fd_memoria;
int fd_filesystem;
char* server_name;

// Variables del config (Las pongo aca asi no estamos revoleando el cfg para todos lados)
char* IP;
char* PUERTO;
char* IP_FILESYSTEM;
char* PUERTO_FILESYSTEM;
char* IP_CPU;
char* PUERTO_CPU;
char* IP_MEMORIA;
char* PUERTO_MEMORIA;
t_algoritmo ALGORITMO_PLANIFICACION;
uint16_t ESTIMACION_INICIAL;
double HRRN_ALFA;
int GRADO_MAX_MULTIPROGRAMACION;
char** RECURSOS;
int* INSTANCIAS_RECURSOS;
t_list* lista_recursos;

// Variables PCBs
int generador_pid;
t_list* lista_ready;
t_list* cola_exit;
t_list* cola_listos_para_ready;
t_list* cola_exec;
t_list* cola_block;
t_list* cola_block_io;
t_list* cola_block_fs;
int fs_mem_op_count;

// Semaforos y pthread
pthread_mutex_t mutex_generador_pid;
pthread_mutex_t mutex_cola_ready;
pthread_mutex_t mutex_cola_listos_para_ready;
pthread_mutex_t mutex_cola_exit;
pthread_mutex_t mutex_cola_exec;
pthread_mutex_t mutex_cola_block;
pthread_mutex_t mutex_cola_block_io;
pthread_mutex_t mutex_cola_block_fs;
sem_t sem_multiprog;
sem_t sem_listos_ready;
sem_t sem_ready;
sem_t sem_exec;
sem_t sem_exit;
sem_t sem_block_return;
sem_t ongoing_fs_mem_op;

// Variables de archivos
t_list* archivos_abiertos;

// INIT
void leer_config();
int* string_to_int_array(char** array_de_strings);
void asignar_algoritmo(char* algoritmo);
bool generar_conexiones();
void inicializar_variables();
t_list* inicializar_recursos();
void inicializar_registro(t_contexto_ejecucion* contexto);
void terminar_programa();

// COMUNICACION
void procesar_conexion(void* args);
void iterator(char* value);
int server_escuchar(int server_socket);
void procesar_conexion_fs(void* void_args);

// PCBS
t_pcb *pcb_create(t_list* instrucciones, int pid, int cliente_socket);
void pcb_destroy(t_pcb* pcb);
void cambiar_estado(t_pcb *pcb, estado_proceso nuevo_estado);
void procesar_cambio_estado(t_pcb* pcb, estado_proceso estado_nuevo);
void armar_pcb(t_list *instrucciones, int cliente_socket);
void actualizar_contexto_pcb(t_pcb* pcb, t_contexto_ejecucion* contexto);
void actualizar_registros(t_pcb* pcb, t_contexto_ejecucion* contexto);
void actualizar_ts_de_pcbs(t_list* lista_ts_wrappers);
void actualizar_ts_de_pcbs_de_cola(t_list* lista_ts_wrappers, t_list* lista_pcb, pthread_mutex_t* mutex_cola);
t_list* get_ts_from_pid(int pid, t_list* lista_ts_wrappers);

// PLANIFICACION
void planificar();
void planificar_largo_plazo();
void exit_pcb();
void ready_pcb();
void block_return_pcb();
t_pcb* safe_pcb_remove(t_list* list, pthread_mutex_t* mutex);
void safe_pcb_add(t_list* list, t_pcb* pcb, pthread_mutex_t* mutex);
void set_pcb_ready(t_pcb* pcb);
void log_cola_ready();
t_list *pcb_to_pid_list(t_list *list);
char* algoritmo_to_string(t_algoritmo algoritmo);
void planificar_corto_plazo();
void exec_pcb();
t_pcb* elegir_pcb_segun_algoritmo();
void dispatch(t_pcb* pcb);
t_pcb* obtener_pcb_HRRN();
bool maximo_HRRN(t_pcb* pcb1, t_pcb* pcb2);
double response_ratio(t_pcb* pcb);
void calcular_estimacion(t_pcb* pcb);
void manejar_io(t_pcb* pcb,  int tiempo);
void exec_io(void* void_arg);
void manejar_wait(t_pcb* pcb, char* recurso);
t_recurso* buscar_recurso(char* recurso);
void manejar_signal(t_pcb* pcb, char* recurso);
void manejar_create_segment(t_pcb* pcb, int cliente_socket, int id_segmento, int tamanio);
t_archivo* get_archivo_global(char* nombre_archivo);
t_archivo* get_archivo_pcb(char* nombre_archivo, t_pcb* pcb);
bool archivo_is_opened(char* nombre_archivo);
t_archivo* archivo_create(char* nombre_archivo);
t_archivo* quitar_archivo_de_tabla_proceso(char* nombre_archivo, t_pcb* pcb);
void ejecutar_f_open(char* nombre_archivo_open, t_pcb* pcb);

#endif /* KERNEL_H_ */

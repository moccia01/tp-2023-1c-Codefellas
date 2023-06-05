#ifndef CPU_H_
#define CPU_H_

#include <commons/log.h>
#include <commons/config.h>
#include <sockets.h>
#include <protocolo.h>
#include <shared.h>
#include <pthread.h>
#include <math.h>


t_log* logger;
t_log* logger_obligatorio;
t_config* config;
t_registros* registros;
//t_contexto_ejecucion* contexto_de_ejecucion;
char* server_name;
int socket_cliente;
int fd_cpu;
int fd_memoria;
bool flag_execute;

// Variables del config (Las pongo aca asi no estamos revoleando el cfg para todos lados)
char* IP;
char* PUERTO;
char* IP_MEMORIA;
char* PUERTO_MEMORIA;
int RETARDO_INSTRUCCION;
int TAM_MAX_SEGMENTO;

//Structs
typedef struct{
	int num_segmento;
	int desplazamiento_segmento;
	int dir_fisica;
	int tamanio;
}t_traduccion_mmu;

// INIT
void leer_config();
void terminar_programa();
void inicializar_variables();

// Comunicacion
static void procesar_conexion();
void server_escuchar() ;

t_registros* inicializar_registro();
void fetch(t_contexto_ejecucion* contexto);
void decode(t_instruccion* proxima_instruccion, t_contexto_ejecucion* contexto);
void ejecutar_ciclo_de_instrucciones(t_contexto_ejecucion* contexto);

//Instrucciones
void ejecutar_set(char* registro, char* valor, t_registros* registros_contexto);
void ejecutar_mov_in(char* registro, int dir_logica, t_contexto_ejecucion* contexto);
void ejecutar_mov_out(int dir_logica, char* registro, t_contexto_ejecucion* contexto);
void ejecutar_io(int tiempo_io, t_contexto_ejecucion* contexto);
void ejecutar_f_open(char* nombre_archivo, t_contexto_ejecucion* contexto);			//Verificar tipo de nombre de archivo
void ejecutar_f_close(char* nombre_archivo, t_contexto_ejecucion* contexto);
void ejecutar_f_seek(char* nombre_archivo, int posicion, t_contexto_ejecucion* contexto);
void ejecutar_f_read(char* nombre_archivo, int dir_logica, int cantidad_bytes, t_contexto_ejecucion* contexto);
void ejecutar_f_write(char* nombre_archivo, int dir_logica, int cantidad_bytes, t_contexto_ejecucion* contexto);
void ejecutar_f_truncate(char* nombre_archivo, int tamanio, t_contexto_ejecucion* contexto);
void ejecutar_wait(char* recurso, t_contexto_ejecucion* contexto);
void ejecutar_signal(char* recurso, t_contexto_ejecucion* contexto);
void ejecutar_create_segment(int id_segmento, int tamanio, t_contexto_ejecucion* contexto);
void ejecutar_delete_segment(int id_segmento, t_contexto_ejecucion* contexto);
void ejecutar_yield(t_contexto_ejecucion* contexto);
void ejecutar_exit(t_contexto_ejecucion* contexto);
void set_valor_registro(char* registro, char* valor);
char* leer_valor_registro(char* registro);
t_traduccion_mmu* traducir_direccion(int dir_logica, t_contexto_ejecucion* contexto);
void actualizar_registros_contexto(t_registros* registros_contexto);
t_segmento* obtener_segmento_de_tabla(t_list* tabla_de_segmentos, int num_segmento);
int obtener_tamanio_registro(char* registro);
void manejar_seg_fault(t_contexto_ejecucion* contexto, t_traduccion_mmu* mmu, int tamanio);

#endif /* CPU_H_ */

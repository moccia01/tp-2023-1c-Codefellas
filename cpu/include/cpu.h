#ifndef CPU_H_
#define CPU_H_

#include <commons/log.h>
#include <commons/config.h>
#include <sockets.h>
#include <protocolo.h>
#include <shared.h>
#include <pthread.h>


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

void leer_config();

// Comunicacion
static void procesar_conexion();
void server_escuchar() ;

t_registros* inicializar_registro();
void fetch(t_contexto_ejecucion* contexto);
void decode(t_instruccion* proxima_instruccion, t_contexto_ejecucion* contexto);
void ejecutar_ciclo_de_instrucciones(t_contexto_ejecucion* contexto);

//Instrucciones	VERIFICAR TIPOS DE RETORNO
void ejecutar_set(char* registro, char* valor);
void ejecutar_mov_in(t_registros registro, int dir_logica);
void ejecutar_mov_out(int dir_logica, t_registros registro);
void ejecutar_io(char* tiempo_io, t_contexto_ejecucion* contexto);
void ejecutar_f_open(char nombre_archivo);			//Verificar tipo de nombre de archivo
void ejecutar_f_close(char nombre_archivo);
void ejecutar_f_seek(char nombre_archivo);
void ejecutar_f_read(char nombre_archivo);
void ejecutar_f_write(char nombre_archivo);
void ejecutar_f_truncate(char nombre_De_Archivo, int tamanio);
void ejecutar_wait();		//Verificar el tipo de recurso
void ejecutar_signal();		//Verificar el tipo de recurso
void ejecutar_create_segment(int id_segmento, int tamanio);
void ejecutar_delete_segment(int id_segmento);
void ejecutar_yield(t_contexto_ejecucion* contexto);
void ejecutar_exit(t_contexto_ejecucion* contexto);

#endif /* CPU_H_ */

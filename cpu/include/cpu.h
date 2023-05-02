#ifndef CPU_H_
#define CPU_H_

#include <commons/log.h>
#include <commons/config.h>
#include <sockets.h>
#include <protocolo.h>
#include <shared.h>

#endif /* CPU_H_ */

typedef enum
{
	SET,
	ADD,
	MOV_IN,
	MOV_OUT,
	IO,
	SIGNAL,
	EXIT,
	UNKNOWN_OP,
	ERROR_MEMORIA,
	WAIT,
	F_OPEN,
	YIELD,
	F_TRUNCATE,
	F_SEEK,
	CREATE_SEGMENT,
	F_WRITE,
	F_READ,
	DELETE_SEGMENT,
	F_CLOSE
} cod_instruccion;

t_log* logger;
t_config* config;
t_registros* registros;

// Variables del config (Las pongo aca asi no estamos revoleando el cfg para todos lados)
char* IP;
char* PUERTO;
char* IP_MEMORIA;
char* PUERTO_MEMORIA;
int RETARDO_INSTRUCCION;

t_registros* inicializar_registro();
void fetch(t_contexto_ejecucion contexto);
void decode(t_list* instruccion);

//Instrucciones	VERIFICAR TIPOS DE RETORNO
void ejecutar_set(char* registro, char* valor);
void ejecutar_mov_in(t_registros registro, int dir_logica);
void ejecutar_mov_out(int dir_logica, t_registros registro);
void ejecutar_i_o(int tiempo);
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
void ejecutar_yield();
void ejecutar_exit();

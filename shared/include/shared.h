#ifndef SHARED_H_
#define SHARED_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include "sockets.h"

typedef struct{
	char* ax;
	char* bx;
	char* cx;
	char* dx;
	char* eax;
	char* ebx;
	char* ecx;
	char* edx;
	char* rax;
	char* rbx;
	char* rcx;
	char* rdx;
} t_registros;

typedef enum{
	NEW,
	READY,
	EXEC,
	BLOCK,
	FINISH_EXIT,
	FINISH_ERROR,
	UNKNOWN_STATE
} estado_proceso;

typedef enum{
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

typedef struct{
	cod_instruccion instruccion;
	char* parametro1;
	char* parametro2;
	char* parametro3;
} t_instruccion;

//MOTIVOS DE BLOQUEO POSIBLES
typedef enum{
	IO_BLOCK,
}motivo_block;

//MOTIVOS DE EXIT
typedef enum{
	SUCCESS,
	SEG_FAULT,
	OUT_OF_MEMORY,
	RECURSO_INEXISTENTE,
}motivo_exit;

typedef struct{
	int id;
	int offset;
	int direccion_fisica;
	int tamanio_segmento;
}t_segmento;

typedef enum{
	SEGMENT_CREATED,
	OUT_OF_MEM,
	COMPACT,
}t_segment_response;

//CONTEXTO DE EJECUCION
typedef struct{
    int pid;
    int program_counter;
    t_list *instrucciones;
    t_list *tabla_de_segmentos;
    estado_proceso estado;
    motivo_exit motivo_exit;
    motivo_block motivo_block;
    t_registros* registros;
    t_segmento* seg_fault;
}t_contexto_ejecucion;

char *estado_to_string(estado_proceso estado);
char *list_to_string(t_list *list);
cod_instruccion instruccion_to_enum(char* instruccion);
void loggear_instrucciones_test(t_log* logger, t_list* instrucciones);
char* instruccion_to_string(t_log* logger, cod_instruccion cod);
char* motivo_exit_to_string(motivo_exit motivo);


#endif /* SHARED_H_ */

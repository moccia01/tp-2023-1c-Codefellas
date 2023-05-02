#ifndef SHARED_H_
#define SHARED_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include "sockets.h"

typedef struct{
	char* instruccion;
	char* parametro1;
	char* parametro2;
	char* parametro3;
} t_instruccion;

typedef struct
{
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

typedef enum
{
	NEW,
	READY,
	EXEC,
	BLOCK,
	FINISH_EXIT,
	FINISH_ERROR,
	UNKNOWN_STATE
} estado_proceso;

//CONTEXTO DE EJECUCION
typedef struct{
    int pid;
    int program_counter;
    t_list *instrucciones;
    t_list *tabla_de_segmentos;
    estado_proceso estado;
}t_contexto_ejecucion;

void terminar_programa(t_log* logger, t_config* config);
char *estado_to_string(estado_proceso estado);

#endif /* SHARED_H_ */

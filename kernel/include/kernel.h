#ifndef KERNEL_H_
#define KERNEL_H_

#include <protocolo.h>
#include <sockets.h>
#include <shared.h>
#include <commons/log.h>
#include <commons/config.h>
#include "init.h"

#endif /* KERNEL_H_ */

typedef struct
{
	char ax;
	char bx;
	char cx;
	char dx;
} registros_de_proposito_general;

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
//sss
typedef enum
{
	FIFO,
	HRRN,
} t_algoritmo;
//Memoria Robado tp Nahu
typedef struct
{
	int indice_tabla_de_pagina;
	int numero_pagina;
} t_pagina;

typedef struct
{
	t_list *instrucciones;
	t_list *segmentos;
} t_proceso;

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
} cod_operacion;

// INSTRUCCIONES QUE TIENEN DOS PARAMETROS
typedef struct
{
	cod_operacion operacion;
	char *parametro1;
	char *parametro2;
} instruccion;


typedef struct
{
	int id;
	int direccion_fisica;
	int tamanio_segmentos;

	}t_segmento;

typedef struct

{
	int pid;
	int program_counter;
	estado_proceso estado;
	//int socket_consola;
	bool interrupcion;
	t_list *instrucciones;
	// punteros de las listas en dudaa
	t_list *tabla_de_segmentos;
	registros_de_proposito_general registros;
	t_pagina *pagina_fault;
	//bool con_desalojo;


} t_pcb;

t_pcb *pcb_create(t_proceso *proceso, int pid, int socket);
void cambiar_estado(t_log* logger, t_pcb *pcb, estado_proceso nuevo_estado);

//instruccion     *obtener_ultima_instruccion(t_pcb *pcb)
//{
//	int ultima_instruccion_idx = pcb->program_counter - 1;
//	instruccion *ultima_instruccion = list_get(pcb->instrucciones, ultima_instruccion_idx);
//return ultima_instruccion;
//}


#ifndef KERNEL_H_
#define KERNEL_H_

#include <protocolo.h>
#include <sockets.h>
#include <shared.h>
#include <commons/log.h>
#include <commons/config.h>
#include "init.h"
#include "planificador.h"
#include <pthread.h>
#include "../include/comunicacion.h"

#endif /* KERNEL_H_ */

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
	//t_list *segmentos;
} t_proceso;


// INSTRUCCIONES QUE TIENEN DOS PARAMETROS
typedef struct
{
	op_code operacion;		//ANTES ERA DE TIPO cod_instruccion
	char *parametro1;
	char *parametro2;
} instruccion;


typedef struct
{
	int id;
	int direccion_fisica;
	int tamanio_segmentos;

}t_segmento;

typedef struct{
	int pid;
	int program_counter;


}t_contexto_ejecucion;

typedef struct
{
	//int pid;
	//int program_counter;
	estado_proceso estado;
	//int socket_consola;
	bool interrupcion;
	t_list *instrucciones;
	// punteros de las listas en dudaa
	t_list *tabla_de_segmentos;
	t_registros registros;
	t_contexto_ejecucion contexto_de_ejecucion;
	t_segmento seg_fault;						//Agregado para la MMU, probablemente vaya en el contexto
	//bool con_desalojo;

} t_pcb;

t_list* lista_pcbs;
t_log* logger;
t_config* config;

t_pcb *pcb_create(t_proceso *proceso, int pid);
void cambiar_estado(t_log* logger, t_pcb *pcb, estado_proceso nuevo_estado);
void armar_pcb(t_list *instrucciones);

//instruccion     *obtener_ultima_instruccion(t_pcb *pcb)
//{
//	int ultima_instruccion_idx = pcb->program_counter - 1;
//	instruccion *ultima_instruccion = list_get(pcb->instrucciones, ultima_instruccion_idx);
//return ultima_instruccion;
//}


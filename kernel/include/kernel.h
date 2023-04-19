#ifndef KERNEL_H_
#define KERNEL_H_

#include <protocolo.h>
#include <sockets.h>
#include <shared.h>
#include <commons/log.h>
#include <commons/config.h>
#include "init.h"

#endif /* KERNEL_H_ */

struct PCB {
	int PID;



};

//void cambiar_estado(t_pcb *pcb, estado_proceso nuevo_estado)
//{
	//char *nuevo_estado_string = strdup(estado_to_string(nuevo_estado));
	//char *estado_anterior_string = strdup(estado_to_string(pcb->estado));
	//log_info(kernel_logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", pcb->pid, estado_anterior_string, nuevo_estado_string);
	//pcb->estado = nuevo_estado;
	//free(estado_anterior_string);
	//free(nuevo_estado_string);
//}

//instruccion *obtener_ultima_instruccion(t_pcb *pcb)
//{
//	int ultima_instruccion_idx = pcb->program_counter - 1;
//	instruccion *ultima_instruccion = list_get(pcb->instrucciones, ultima_instruccion_idx);
//	return ultima_instruccion;
//}


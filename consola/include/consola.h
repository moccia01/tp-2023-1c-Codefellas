#ifndef CONSOLA_H_
#define CONSOLA_H_

#include <protocolo.h>
#include <sockets.h>
#include <shared.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <string.h>
#define MAX_LINE_LENGTH 256

#endif /* CONSOLA_H_ */

typedef struct{
	char* instruccion;
	char* parametro1;
	char* parametro2;
	char* parametro3;
} t_instruccion;

t_list* leer_instrucciones(char* path, t_log* logger);

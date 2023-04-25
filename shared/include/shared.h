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

void terminar_programa(t_log* logger, t_config* config);

#endif /* SHARED_H_ */

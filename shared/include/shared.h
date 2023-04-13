#ifndef SHARED_H_
#define SHARED_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include "sockets.h"

void terminar_programa(t_log* logger, t_config* config, int conexion);

#endif /* SHARED_H_ */

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

t_log* logger;
t_config* config;

#endif /* CONSOLA_H_ */

t_list* leer_instrucciones(char* path, t_log* logger);

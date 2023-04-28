#ifndef INIT_H_
#define INIT_H_

#include <sockets.h>
#include <protocolo.h>
#include <commons/config.h>

bool generar_conexiones(t_log* logger, t_config* config, int* fd_cpu, int* fd_memoria, int* fd_filesystem);
int inicializar_servidor(t_log* logger, t_config* config);

#endif /* INIT_H_ */

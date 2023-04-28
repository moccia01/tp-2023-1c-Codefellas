#ifndef INIT_H_
#define INIT_H_

#include <sockets.h>
#include <protocolo.h>
#include <commons/config.h>

int inicializar_servidor(t_log* logger, t_config* config);
void recibir_conexiones(t_log* logger, t_config* config,int fd_memoria, int* fd_cpu, int* fd_filesystem, int* fd_kernel);
void procesar_conexiones(t_log* logger, int fd_cpu, int fd_filesystem, int fd_kernel);

#endif /* INIT_H_ */

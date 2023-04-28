#include "../include/init.h"

// esto claramente no queda asi, es para testear hasta q implementemos hilos nomas
void recibir_conexiones(t_log* logger, t_config* config, int fd_memoria, int* fd_cpu, int* fd_filesystem, int* fd_kernel){
	// TODO: Implementar servidor multihilo para escuchar varios clientes.
//	*fd_cpu = esperar_cliente(logger, fd_memoria);
//	*fd_filesystem = esperar_cliente(logger, fd_memoria);
//	*fd_kernel = esperar_cliente(logger, fd_memoria);
}

// Claramente esto tampoco queda asi, creo q deberia tener un switch con todos los posibles op_code y deberia ser individual para cada hilo
void procesar_conexiones(t_log* logger, int fd_cpu, int fd_filesystem, int fd_kernel){
	// Recibo Operacion de cpu
		op_code cod_op_cpu = recibir_operacion(fd_cpu);
		if(cod_op_cpu != MENSAJE){
			log_error(logger, "no se q paso pero exploto\n");
			exit(1);
		}
		recibir_mensaje(logger, fd_cpu);

		// Recibo Operacion de Filesystem
		op_code cod_op_filesystem = recibir_operacion(fd_filesystem);
		if(cod_op_filesystem != MENSAJE){
			log_error(logger, "no se q paso pero exploto\n");
			exit(1);
		}
		recibir_mensaje(logger, fd_filesystem);

		// Recibo Operacion de Kernel
		op_code cod_op_kernel= recibir_operacion(fd_kernel);
		if(cod_op_kernel != MENSAJE){
			log_error(logger, "no se q paso pero exploto\n");
			exit(1);
		}
		recibir_mensaje(logger, fd_kernel);
}

#include "../include/init.h"

bool generar_conexiones(t_log* logger, t_config* config, int* fd_cpu, int* fd_memoria, int* fd_filesystem){
	// Leo IPs y Puertos del config
	char* IP_FILESYSTEM = config_get_string_value(config, "IP_FILESYSTEM");
	char* PUERTO_FILESYSTEM = config_get_string_value(config, "PUERTO_FILESYSTEM");
	char* IP_CPU = config_get_string_value(config, "IP_CPU");
	char* PUERTO_CPU = config_get_string_value(config, "PUERTO_CPU");
	char* IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
	char* PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");

	*fd_filesystem = crear_conexion(IP_FILESYSTEM, PUERTO_FILESYSTEM);
	*fd_cpu = crear_conexion(IP_CPU, PUERTO_CPU);
	*fd_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);

	return *fd_filesystem != 0 && *fd_cpu != 0 && *fd_memoria != 0;
}

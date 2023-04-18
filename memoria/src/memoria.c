#include "../include/memoria.h"

int main(void) {
	t_log* logger = log_create("memoria.log", "memoria_main", true, LOG_LEVEL_INFO);
	t_config* config = config_create("memoria.config");

	if(config == NULL){
		log_error(logger, "No se encontró el archivo :(");
		exit(1);
	}

	// Inicio servidor
	char* IP = config_get_string_value(config, "IP_ESCUCHA");
	char* PUERTO = config_get_string_value(config, "PUERTO_ESCUCHA");
	int fd_memoria = iniciar_servidor(logger, IP, PUERTO);

	int fd_cpu, fd_filesystem, fd_kernel;

	recibir_conexiones(logger, config, fd_memoria, &fd_cpu, &fd_filesystem, &fd_kernel);

	procesar_conexiones(logger, fd_cpu, fd_filesystem, fd_kernel);

	// Termino programa
	terminar_programa(logger, config);
	liberar_conexion(fd_cpu);
	liberar_conexion(fd_kernel);
	liberar_conexion(fd_filesystem);
	return 0;
}

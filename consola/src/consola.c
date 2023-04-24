#include "../include/consola.h"

int main(void) {


	t_log* logger = log_create("consola.log", "consola_main", true, LOG_LEVEL_INFO);
	t_config* config = config_create("consola.config");

	if(config == NULL){
		log_error(logger, "No se encontró el archivo :(");
		exit(1);
	}

	char* IP_KERNEL = config_get_string_value(config, "IP_KERNEL");
	// El enunciado dice q el puerto es numerico, pero la funcion getaddrinfo recibe char* y no puedo castear de int a char* :(
	char* PUERTO_KERNEL = config_get_string_value(config, "PUERTO_KERNEL");

	// Conexion Kernel
	int fd_kernel = crear_conexion(IP_KERNEL, PUERTO_KERNEL);
	enviar_mensaje("Hola, soy consola.", fd_kernel);

	terminar_programa(logger, config);
	liberar_conexion(fd_kernel);

	return 0;
}

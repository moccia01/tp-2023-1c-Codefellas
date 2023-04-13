#include "../include/consola.h"

int main(void) {

	t_log* logger = log_create("consola.log", "consola_main", true, LOG_LEVEL_INFO);
	t_config* config = config_create("consola.config");

	if(config == NULL){
		printf("No se encontro el archivo\n");
		exit(1);
	}

	// Conexion Kernel
	char* IP_KERNEL = config_get_string_value(config, "IP_KERNEL");
	// El enunciado dice q el puerto es numerico, pero la funcion getaddrinfo recibe char* y no puedo castear de int a char* :(
	char* PUERTO_KERNEL = config_get_string_value(config, "PUERTO_KERNEL");
	int conexion = crear_conexion(IP_KERNEL, PUERTO_KERNEL);
	enviar_mensaje("Hola, soy consola.", conexion);

	terminar_programa(logger, config, conexion);

	return 0;
}

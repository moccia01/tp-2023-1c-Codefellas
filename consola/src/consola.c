#include "../include/consola.h"

int main(void) {

	t_log* logger = log_create("consola.log", "main", true, LOG_LEVEL_INFO);
	t_config* config = config_create("consola.config");

	if(config == NULL){
		printf("No se encontro el archivo\n");
		exit(1);
	}

	char* ip = config_get_string_value(config, "IP_KERNEL");
	char* puerto = config_get_string_value(config, "PUERTO_KERNEL");

	int conexion = crear_conexion(ip, puerto);

	enviar_mensaje("Hola, soy consola.", conexion);

	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(conexion);

	return 0;
}

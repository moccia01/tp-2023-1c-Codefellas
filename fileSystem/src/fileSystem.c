#include "../include/fileSystem.h"

int main(void) {
	t_log* logger = log_create("filesystem.log", "filesystem_main", 1, LOG_LEVEL_INFO);
	t_config* config = config_create("filesystem.config");

	// Inicio de servidor
	char* IP = config_get_string_value(config, "IP_ESCUCHA");
	char* PUERTO = config_get_string_value(config, "PUERTO_ESCUCHA");
	int fd_filesystem = iniciar_servidor(logger, IP, PUERTO);

	// Conexion Kernel
	int fd_kernel = esperar_cliente(logger, fd_filesystem);
	int cod_op = recibir_operacion(fd_kernel);
	if(cod_op != MENSAJE){
		printf("no se q paso pero exploto\n");
		exit(1);
	}
	recibir_mensaje(logger, fd_kernel);

	terminar_programa(logger, config, 0);
	return 0;
}

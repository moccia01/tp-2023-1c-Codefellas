#include "../include/kernel.h"

int main(void) {
	t_log* logger = log_create("kernel.log", "kernel_main", 1, LOG_LEVEL_INFO);
	t_config* config = config_create("kernel.config");

	// Inicio de servidor
	char* IP = config_get_string_value(config, "IP_ESCUCHA");
	char* PUERTO = config_get_string_value(config, "PUERTO_ESCUCHA");
	int fd_kernel = iniciar_servidor(logger, IP, PUERTO);

	// Conexion consola
	int fd_consola = esperar_cliente(logger, fd_kernel);
	int cod_op = recibir_operacion(fd_consola);
	if(cod_op != MENSAJE){
		printf("no se q paso pero exploto\n");
		exit(1);
	}
	recibir_mensaje(logger, fd_consola);

	// Conexion Filesystem
	char* IP_FILESYSTEM = config_get_string_value(config, "IP_FILESYSTEM");
	char* PUERTO_FILESYSTEM = config_get_string_value(config, "PUERTO_FILESYSTEM");
	int conexion_filesystem = crear_conexion(IP_FILESYSTEM, PUERTO_FILESYSTEM);
	enviar_mensaje("Hola, soy kernel.", conexion_filesystem);

	// Conexion CPU
	char* IP_CPU = config_get_string_value(config, "IP_CPU");
	char* PUERTO_CPU = config_get_string_value(config, "PUERTO_CPU");
	int conexion_cpu = crear_conexion(IP_CPU, PUERTO_CPU);
	enviar_mensaje("Hola, soy kernel.", conexion_cpu);

	// Conexion memoria

	terminar_programa(logger, config, conexion_cpu);
	liberar_conexion(conexion_filesystem);
	return 0;
}

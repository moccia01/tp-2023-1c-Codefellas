#include "../include/cpu.h"

int main(void) {
	t_log* logger = log_create("cpu.log", "cpu_main", 1, LOG_LEVEL_INFO);
	t_config* config = config_create("cpu.config");

	if(config == NULL){
		log_error(logger, "No se encontró el archivo :(");
		exit(1);
	}

	char* IP = config_get_string_value(config, "IP_ESCUCHA");
	char* PUERTO = config_get_string_value(config, "PUERTO_ESCUCHA");
	char* IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
	char* PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");

	// Inicio de servidor
	int fd_cpu = iniciar_servidor(logger, IP, PUERTO);

	// Conexion Kernel
	int fd_kernel = esperar_cliente(logger, "CPU", fd_cpu);
	int cod_op = recibir_operacion(fd_kernel);
	if(cod_op != MENSAJE){
		log_error(logger, "no se q paso pero exploto\n");
		exit(1);
	}
	recibir_mensaje(logger, fd_kernel);

	// Conecto CPU con memoria
	int fd_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
	enviar_mensaje("Hola, soy CPU!", fd_memoria);

	terminar_programa(logger, config);

	return 0;
}

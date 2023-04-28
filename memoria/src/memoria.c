#include "../include/memoria.h"

int main(void) {
	logger = log_create("memoria.log", "memoria_main", true, LOG_LEVEL_INFO);
	config = config_create("memoria.config");

	if(config == NULL){
		log_error(logger, "No se encontró el archivo :(");
		exit(1);
	}

	// Inicio servidor
	int fd_memoria = inicializar_servidor(logger, config);

	while(server_escuchar(logger, fd_memoria));

	// Termino programa
	terminar_programa(logger, config);

	return 0;
}

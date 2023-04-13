#include "../include/memoria.h"

int main(void) {
	//t_log* logger = log_create("memoria.log", "main", true, LOG_LEVEL_INFO);
	t_config* config = config_create("memoria.config");

	if(config == NULL){
			printf("No se encontro el archivo\n");
			exit(1);
		}

//	char* IP = config_get_string_value(config, "PUERTO_ESCUCHA");
//	char* PUERTO = config_get_string_value(config, "PUERTO_ESCUCHA");
//	int fd_memoria = iniciar_servidor(logger, IP, PUERTO);

	// TODO: Implementar servidor multihilo para escuchar varios clientes.





	return 0;
}

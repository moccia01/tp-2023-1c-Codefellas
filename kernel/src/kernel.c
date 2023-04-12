#include "../include/kernel.h"

int main(void) {
	t_log* logger = log_create("kernel.log", "main", 1, LOG_LEVEL_INFO);

	int kernel = iniciar_servidor();
	log_info(logger, "Listo para recibir mensaje de consola");
	int consola = esperar_cliente(kernel);

	int cod_op = recibir_operacion(consola);
	if(cod_op != MENSAJE){
		printf("no se q paso pero exploto\n");
		exit(1);
	}
	recibir_mensaje(consola);

	return 0;
}

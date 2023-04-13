#include "../include/shared.h"

void terminar_programa(t_log* logger, t_config* config, int conexion){
	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(conexion);
}

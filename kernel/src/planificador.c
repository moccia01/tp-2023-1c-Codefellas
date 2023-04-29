#include "../include/planificador.h"

void planificar(t_log* logger, t_config* config){
	planificar_largo_plazo(logger, config);
	log_info(logger, "Se inició la planificacion de largo plazo");
	planificar_corto_plazo(logger, config);
	log_info(logger, "Se inició la planificacion de corto plazo");
}

void planificar_largo_plazo(t_log* logger, t_config* config){
	//int grado_multiprog = config_get_int_value(config, "GRADO_MAX_MULTIPROGRAMACION");

}

void planificar_corto_plazo(t_log* logger, t_config* config){
	//char* algoritmo = config_get_string_value(config, "ALGORITMO");

}

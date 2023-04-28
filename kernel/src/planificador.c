#include "../include/planificador.h"

void planificar(t_config* config){
	planificar_largo_plazo(config);

	planificar_corto_plazo(config);

}

void planificar_largo_plazo(t_config* config){
	//int grado_multiprog = config_get_int_value(config, "GRADO_MAX_MULTIPROGRAMACION");

}

void planificar_corto_plazo(t_config* config){
	//char* algoritmo = config_get_string_value(config, "ALGORITMO");

}

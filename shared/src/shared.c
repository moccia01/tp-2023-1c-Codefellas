#include "../include/shared.h"

void terminar_programa(t_log* logger, t_config* config){
	log_destroy(logger);
	config_destroy(config);
}

char *estado_to_string(estado_proceso estado)
{
    switch (estado)
    {
    case NEW:
        return "NEW";
        break;
    case READY:
        return "READY";
        break;
    case BLOCK:
        return "BLOCK";
        break;
    case EXEC:
        return "EXEC";
        break;
    case FINISH_EXIT:
        return "EXIT";
        break;
    case FINISH_ERROR:
        return "ERROR";
        break;
    default:
        return "UNKNOWN";
        break;
    }
}

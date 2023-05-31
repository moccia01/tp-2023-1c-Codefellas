#include "../include/shared.h"

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

char *list_to_string(t_list *list)
{
    char *string = string_new();
    for (int i = 0; i < list_size(list); i++)
    {
        int *num = (int *)list_get(list, i);
        if (i < list_size(list) - 1)
        {
            string_append_with_format(&string, "%d,", *num);
        }
        else
        {
            string_append_with_format(&string, "%d", *num);
        }
    }
    return string;
}

cod_instruccion instruccion_to_enum(char* instruccion){

	if(strcmp(instruccion, "SET") == 0){
		return SET;
	} else if(strcmp(instruccion, "MOV_IN") == 0){
		return MOV_IN;
	} else if(strcmp(instruccion, "MOV_OUT") == 0){
		return MOV_OUT;
	}else if(strcmp(instruccion, "I/O") == 0){
		return IO;
	} else if(strcmp(instruccion, "F_OPEN") == 0){
		return F_OPEN;
	} else if(strcmp(instruccion, "F_CLOSE") == 0){
		return F_CLOSE;
	} else if(strcmp(instruccion, "F_SEEK") == 0){
		return F_SEEK;
	} else if(strcmp(instruccion, "F_READ") == 0){
		return F_READ;
	} else if(strcmp(instruccion, "F_WRITE") == 0){
		return F_WRITE;
	} else if(strcmp(instruccion, "F_TRUNCATE") == 0){
		return F_TRUNCATE;
	} else if(strcmp(instruccion, "WAIT") == 0){
		return WAIT;
	} else if(strcmp(instruccion, "SIGNAL") == 0){
		return SIGNAL;
	} else if(strcmp(instruccion, "CREATE_SEGMENT") == 0){
		return CREATE_SEGMENT;
	} else if(strcmp(instruccion, "DELETE_SEGMENT") == 0){
		return DELETE_SEGMENT;
	} else if(strcmp(instruccion, "YIELD") == 0){
		return YIELD;
	} else if(strcmp(instruccion, "EXIT") == 0){
		return EXIT;
	}
	return -1;
}

void loggear_instrucciones_test(t_log* logger, t_list* instrucciones){
	log_info(logger, "el tamaño de la lista es %d", list_size(instrucciones));
	for(int i = 0; i<list_size(instrucciones); i++){
		t_instruccion* instruccion = list_get(instrucciones, i);
		char* cod = instruccion_to_string(logger, instruccion->instruccion);
		log_info(logger, "La instruccion %d es: %s %s %s %s", i + 1, cod, instruccion->parametro1, instruccion->parametro2, instruccion-> parametro3);
	}
}

char* instruccion_to_string(t_log* logger, cod_instruccion cod){
	switch(cod){
			case SET: return "SET";
				break;
			case MOV_IN: return "MOV_IN";
				break;
			case MOV_OUT:return "MOV_OUT";
				break;
			case IO:return "I/O";
				break;
			case F_OPEN:return "F_OPEN";
				break;
			case F_CLOSE:return "F_CLOSE";
				break;
			case F_SEEK:return "F_SEEK";
				break;
			case F_READ:return "F_READ";
				break;
			case F_WRITE:return "F_WRITE";
				break;
			case F_TRUNCATE:return "F_TRUNCATE";
				break;
			case WAIT:return "WAIT";
				break;
			case SIGNAL:return "SIGNAL";
				break;
			case CREATE_SEGMENT:return "CREATE_SEGMENT";
				break;
			case DELETE_SEGMENT:return "DELETE_SEGMENT";
				break;
			case YIELD:return "YIELD";
				break;
			case EXIT:return "EXIT";
				break;
			default:
				log_error(logger, "Instruccion no reconocida");
				return NULL;
		}
}

char* motivo_exit_to_string(motivo_exit motivo){
	switch(motivo){
	case SUCCESS: return "SUCCESS";
	case SEG_FAULT: return "SEG_FAULT";
	case OUT_OF_MEMORY: return "OUT_OF_MEMORY";
	case RECURSO_INEXISTENTE: return "RECURSO_INEXISTENTE";
	default: return "INDETERMINADO";
	}
}



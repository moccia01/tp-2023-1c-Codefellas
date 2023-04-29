#include "../include/cpu.h"

int main(void) {
	t_log* logger = log_create("cpu.log", "cpu_main", 1, LOG_LEVEL_INFO);
	t_config* config = config_create("cpu.config");
	t_registros* registros = inicializar_registro();

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

t_registros* inicializar_registro(){
	t_registros* registro = malloc(sizeof(t_registros));

	//FUNCÓ PERO SUPONGO QUE HAY QUE CAMBIAR EL SIZE OF
	registro->ax = malloc(sizeof(int));		//CHEQUEAR SI SON 4 BYTES
	registro->bx = malloc(sizeof(int));
	registro->cx = malloc(sizeof(int));
	registro->dx = malloc(sizeof(int));
	registro->eax = malloc(sizeof(__uint8_t));
	registro->ebx = malloc(sizeof(__uint8_t));
	registro->ecx = malloc(sizeof(__uint8_t));
	registro->edx = malloc(sizeof(__uint8_t));
	registro->rax = malloc(sizeof(__uint16_t));
	registro->rbx = malloc(sizeof(__uint16_t));
	registro->rcx = malloc(sizeof(__uint16_t));
	registro->rdx = malloc(sizeof(__uint16_t));

	registro->ax = NULL;
	registro->bx = NULL;
	registro->cx = NULL;
	registro->dx = NULL;
	registro->eax = NULL;
	registro->ebx = NULL;
	registro->ecx = NULL;
	registro->edx = NULL;
	registro->rax = NULL;
	registro->rbx = NULL;
	registro->rcx = NULL;
	registro->rdx = NULL;

	return registro;
}

void fetch(){

}

void decode(){


	switch(cod_operacion){
	case "SET":
		break;
	case "MOV_IN":
		break;
	case "I/O":
		break;
	case "F_OPEN":
		break;
	case "F_CLOSE":
		break;
	case "F_SEEK":
		break;
	case "F_READ":
		break;
	case "F_WRITE":
		break;
	case "F_TRUNCATE":
		break;
	case "WAIT":
		break;
	case "SIGNAL":
		break;
	case "CREATE_SEGMENT":
		break;
	case "DELETE_SEGMENT":
		break;
	case "YIELD":
		yield();
		break;
	case "EXIT":
		exit();
		break;
	default:
		log_error(logger, "Instruccion no reconocida");
		return;
	}
}

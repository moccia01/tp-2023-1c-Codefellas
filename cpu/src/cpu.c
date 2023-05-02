#include "../include/cpu.h"

int main(void) {
	logger = log_create("cpu.log", "cpu_main", 1, LOG_LEVEL_INFO);
	config = config_create("cpu.config");
	registros = inicializar_registro();

	if(config == NULL){
		log_error(logger, "No se encontró el archivo :(");
		exit(1);
	}

	IP = config_get_string_value(config, "IP_ESCUCHA");
	PUERTO = config_get_string_value(config, "PUERTO_ESCUCHA");
	IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
	PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");

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
	registros = malloc(sizeof(t_registros));

	//FUNCÓ PERO SUPONGO QUE HAY QUE CAMBIAR EL SIZE OF
	registros->ax = malloc(sizeof(5));
	registros->bx = malloc(sizeof(5));
	registros->cx = malloc(sizeof(5));
	registros->dx = malloc(sizeof(5));
	registros->eax = malloc(sizeof(9));
	registros->ebx = malloc(sizeof(9));
	registros->ecx = malloc(sizeof(9));
	registros->edx = malloc(sizeof(9));
	registros->rax = malloc(sizeof(17));
	registros->rbx = malloc(sizeof(17));
	registros->rcx = malloc(sizeof(17));
	registros->rdx = malloc(sizeof(17));

	registros->ax = NULL;
	registros->bx = NULL;
	registros->cx = NULL;
	registros->dx = NULL;
	registros->eax = NULL;
	registros->ebx = NULL;
	registros->ecx = NULL;
	registros->edx = NULL;
	registros->rax = NULL;
	registros->rbx = NULL;
	registros->rcx = NULL;
	registros->rdx = NULL;

	return registros;
}

void fetch(t_contexto_ejecucion contexto){
	t_list* proxima_instruccion = contexto.instrucciones;		//t_instruccion???
	list_get(proxima_instruccion, contexto.program_counter);

	decode(proxima_instruccion); // ver despues para poner como tipo instrucion

	contexto.program_counter += 1;

}

void decode(t_list* instruccion){ // ver despues para poner como tipo instrucion
	cod_instruccion cod_instruccion;	//NO VA

	switch(cod_instruccion){
	case SET:
		break;
	case MOV_IN:
		break;
	case IO:
		break;
	case F_OPEN:
		break;
	case F_CLOSE:
		break;
	case F_SEEK:
		break;
	case F_READ:
		break;
	case F_WRITE:
		break;
	case F_TRUNCATE:
		break;
	case WAIT:
		break;
	case SIGNAL:
		break;
	case CREATE_SEGMENT:
		break;
	case DELETE_SEGMENT:
		break;
	case YIELD:
		//ejecutar_yield();
		break;
	case EXIT:
		//ejecutar_exit();
		break;
	default:
		log_error(logger, "Instruccion no reconocida");
		return;
	}
}

void ejecutar_set(char* registro, char* valor){

	strcat(valor, "\0");

	if(strcmp(registro, "AX") == 0){
		registros->ax = valor;
	}else if(strcmp(registro, "BX") == 0){
		registros->bx = valor;
	}else if(strcmp(registro, "CX") == 0){
		registros->cx = valor;
	}else if(strcmp(registro, "DX") == 0){
		registros->dx = valor;
	}else if(strcmp(registro, "EAX") == 0){
		registros->eax = valor;
	}else if(strcmp(registro, "EBX") == 0){
		registros->ebx = valor;
	}else if(strcmp(registro, "ECX") == 0){
		registros->ecx = valor;
	}else if(strcmp(registro, "EDX") == 0){
		registros->edx = valor;
	}else if(strcmp(registro, "RAX") == 0){
		registros->rax = valor;
	}else if(strcmp(registro, "RBX") == 0){
		registros->rbx = valor;
	}else if(strcmp(registro, "RCX") == 0){
		registros->rcx = valor;
	}else if(strcmp(registro, "RDX") == 0){
		registros->rdx = valor;
	}

	sleep(RETARDO_INSTRUCCION);
	//VER EL TEMA DEL SLEEP PARA EL RETARDO DE LA INSTRUCCION

}

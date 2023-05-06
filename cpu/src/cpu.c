#include "../include/cpu.h"

int main(void) {
	logger = log_create("cpu.log", "cpu_main", 1, LOG_LEVEL_INFO);
	logger_obligatorio = log_create("cpu.log", "cpu_obligatorio", 1, LOG_LEVEL_INFO);
	config = config_create("cpu.config");
	registros = inicializar_registro();

	//Inicializar variables (CUANDO ESCALE PASARLO A UNA FUNCION)
	flag_execute = true;

	if(config == NULL){
		log_error(logger, "No se encontró el archivo :(");
		exit(1);
	}
	leer_config();

	// Conecto CPU con memoria
	fd_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
	enviar_mensaje("Hola, soy CPU!", fd_memoria);

	// Inicio de servidor
	fd_cpu = iniciar_servidor(logger, IP, PUERTO);

	// Conexion Kernel
	pthread_t conexion_kernel;
	pthread_create(&conexion_kernel, NULL, (void*) server_escuchar, NULL);
	pthread_join(conexion_kernel, NULL);

	terminar_programa(logger, config);
	return 0;
}

void leer_config(){
	IP = config_get_string_value(config, "IP_ESCUCHA");
	PUERTO = config_get_string_value(config, "PUERTO_ESCUCHA");
	IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
	PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");
}

// --------------- COMUNICACION ---------------

static void procesar_conexion() {
	op_code cop;
	while (socket_cliente != -1) {
        if (recv(socket_cliente, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
            log_info(logger, "El cliente se desconecto de %s server", server_name);
            return;
        }
		switch (cop) {
		case MENSAJE:
			recibir_mensaje(logger, socket_cliente);
			break;
		case CONTEXTO_EJECUCION:
			t_contexto_ejecucion* contexto_de_ejecucion = recv_contexto_ejecucion(socket_cliente);
			log_info(logger, "recibí el contexto del proceso %d y se inicia el ciclo de instruccion", contexto_de_ejecucion->pid);
			ejecutar_ciclo_de_instrucciones(contexto_de_ejecucion);
			//socket_cliente = fd_kernel
			break;
		// ...
		default:
			log_error(logger, "Algo anduvo mal en el server de %s", server_name);
			return;
		}
	}

	log_warning(logger, "El cliente se desconecto de %s server", server_name);
	return;
}

void server_escuchar() {
	char *server_name = "CPU";
	socket_cliente = esperar_cliente(logger, server_name, fd_cpu);

	if (socket_cliente == -1) {
		log_info(logger, "Hubo un error en la conexion del Kernel");
	}
	procesar_conexion();
}


// --------------- CICLO DE INSTRUCCIONES ---------------
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

void fetch(t_contexto_ejecucion* contexto){
	log_info(logger, "se ejecuta fetch");
	t_instruccion* proxima_instruccion = list_get(contexto->instrucciones, contexto->program_counter);
	log_info(logger, "1");
	contexto->program_counter += 1;
	log_info(logger, "1");
	decode(proxima_instruccion, contexto);
}

void decode(t_instruccion* proxima_instruccion, t_contexto_ejecucion* contexto){
	log_info(logger, "se ejecuta decode");
	cod_instruccion cod_instruccion = proxima_instruccion->instruccion;

	//los logs son para testear e ir sabiendo lo que se va ejecutando
	switch(cod_instruccion){
		case SET:
			ejecutar_set(proxima_instruccion->parametro1, proxima_instruccion->parametro2);
			log_info(logger,"Se esta ejecutando un SET");
			break;
		case MOV_IN:
			log_info(logger,"Se esta ejecutando un MOV_IN");
			break;
		case MOV_OUT:
			log_info(logger,"Se esta ejecutando un MOV_OUT");
			break;
		case IO:
			flag_execute = false;
			log_info(logger,"Se esta ejecutando un I/O");
			break;
		case F_OPEN:
			log_info(logger,"Se esta ejecutando un F_OPEN");
			break;
		case F_CLOSE:
			log_info(logger,"Se esta ejecutando un F_CLOSE");
			break;
		case F_SEEK:
			log_info(logger,"Se esta ejecutando un F_SEEK");
			break;
		case F_READ:
			log_info(logger,"Se esta ejecutando un F_READ");
			break;
		case F_WRITE:
			log_info(logger,"Se esta ejecutando un F_WRITE");
			break;
		case F_TRUNCATE:
			log_info(logger,"Se esta ejecutando un F_TRUNCATE");
			break;
		case WAIT:
			log_info(logger,"Se esta ejecutando un WAIT");
			break;
		case SIGNAL:
			log_info(logger,"Se esta ejecutando un SIGNAL");
			break;
		case CREATE_SEGMENT:
			log_info(logger,"Se esta ejecutando un CREATE_SEGMENT");
			break;
		case DELETE_SEGMENT:
			log_info(logger,"Se esta ejecutando un DELETE_SEGMENT");
			break;
		case YIELD:
			flag_execute = false;
			ejecutar_yield(contexto);
			log_info(logger,"Se esta ejecutando un YIELD");
			break;
		case EXIT:
			flag_execute = false;
			ejecutar_exit(contexto);
			log_info(logger,"Se esta ejecutando un EXIT");
			break;
		default:
			log_error(logger, "Instruccion no reconocida");
			return;
	}
}

void ejecutar_set(char* registro, char* valor){
	// les falta asignar el \0 al final de cada una
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

	log_info(logger, "a mimir");
	sleep(RETARDO_INSTRUCCION);
	//VER EL TEMA DEL SLEEP PARA EL RETARDO DE LA INSTRUCCION

}

void ejecutar_yield(t_contexto_ejecucion* contexto){
	contexto->estado = YIELD;
	// Avisarle al kernel q ponga al proceso asociado al contexto de ejecucion en ready.
	send_cambiar_estado(contexto, socket_cliente);
}

void ejecutar_exit(t_contexto_ejecucion* contexto){
	contexto->estado = EXIT;
	contexto->motivo_exit = SUCCESS;
	send_cambiar_estado(contexto, socket_cliente);
	// Avisarle al kernel q ponga al proceso asociado al contexto de ejecucion en exit.
}

void ejecutar_ciclo_de_instrucciones(t_contexto_ejecucion* contexto){
	while(flag_execute){
		fetch(contexto);
	}

}

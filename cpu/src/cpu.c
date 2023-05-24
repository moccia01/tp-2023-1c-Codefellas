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
//	fd_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
//	enviar_mensaje("Hola, soy CPU!", fd_memoria);

	// Inicio de servidor
	fd_cpu = iniciar_servidor(logger, IP, PUERTO);
	server_escuchar();
	// Conexion Kernel
//	pthread_t conexion_kernel;
//	pthread_create(&conexion_kernel, NULL, (void*) server_escuchar, NULL);
//	pthread_join(conexion_kernel, NULL);

	terminar_programa(logger, config);
	return 0;
}

void leer_config(){
	IP = config_get_string_value(config, "IP_ESCUCHA");
	PUERTO = config_get_string_value(config, "PUERTO_ESCUCHA");
	IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
	PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");
	RETARDO_INSTRUCCION = config_get_int_value(config, "RETARDO_INSTRUCCION");
	TAM_MAX_SEGMENTO = config_get_int_value(config, "TAM_MAX_SEGMENTO");
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
			flag_execute = true;
			ejecutar_ciclo_de_instrucciones(contexto_de_ejecucion);
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
	server_name = "CPU";
	socket_cliente = esperar_cliente(logger, server_name, fd_cpu);

	if (socket_cliente == -1) {
		log_info(logger, "Hubo un error en la conexion del Kernel");
	}
	procesar_conexion();
}


// --------------- CICLO DE INSTRUCCIONES ---------------
t_registros* inicializar_registro(){
	registros = malloc(sizeof(t_registros));

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
	t_instruccion* proxima_instruccion = list_get(contexto->instrucciones, contexto->program_counter);
	contexto->program_counter += 1;
	decode(proxima_instruccion, contexto);
}

void decode(t_instruccion* proxima_instruccion, t_contexto_ejecucion* contexto){
	cod_instruccion cod_instruccion = proxima_instruccion->instruccion;
	int tiempo;
	int posicion;
	int dir_logica;
	int cantidad_bytes;
	int tamanio;
	int id_segmento;

	//los logs son para testear e ir sabiendo lo que se va ejecutando
	switch(cod_instruccion){
		case SET:
			ejecutar_set(proxima_instruccion->parametro1, proxima_instruccion->parametro2);
			log_info(logger,"Se esta ejecutando un SET");
			break;
		case MOV_IN:
			dir_logica = atoi(proxima_instruccion->parametro2);
			ejecutar_mov_in(proxima_instruccion->parametro1, dir_logica, contexto);
			log_info(logger,"Se esta ejecutando un MOV_IN");
			break;
		case MOV_OUT:
			dir_logica = atoi(proxima_instruccion->parametro1);
			ejecutar_mov_out(dir_logica, proxima_instruccion->parametro2);
			log_info(logger,"Se esta ejecutando un MOV_OUT");
			break;
		case IO:
			flag_execute = false;
			tiempo = atoi(proxima_instruccion->parametro1);
			ejecutar_io(tiempo, contexto);
			log_info(logger,"Se esta ejecutando un I/O");
			break;
		case F_OPEN:
			flag_execute = false;
			ejecutar_f_open(proxima_instruccion->parametro1, contexto);
			log_info(logger,"Se esta ejecutando un F_OPEN");
			break;
		case F_CLOSE:
			flag_execute = false;
			ejecutar_f_close(proxima_instruccion->parametro1, contexto);
			log_info(logger,"Se esta ejecutando un F_CLOSE");
			break;
		case F_SEEK:
			flag_execute = false;
			posicion = atoi(proxima_instruccion->parametro2);
			ejecutar_f_seek(proxima_instruccion->parametro1, posicion, contexto);
			log_info(logger,"Se esta ejecutando un F_SEEK");
			break;
		case F_READ:
			flag_execute = false;
			dir_logica = atoi(proxima_instruccion->parametro2);
			cantidad_bytes = atoi(proxima_instruccion->parametro3);
			ejecutar_f_read(proxima_instruccion->parametro1, dir_logica, cantidad_bytes, contexto);
			log_info(logger,"Se esta ejecutando un F_READ");
			break;
		case F_WRITE:
			flag_execute = false;
			dir_logica = atoi(proxima_instruccion->parametro2);
			cantidad_bytes = atoi(proxima_instruccion->parametro3);
			ejecutar_f_write(proxima_instruccion->parametro1, dir_logica, cantidad_bytes, contexto);
			log_info(logger,"Se esta ejecutando un F_WRITE");
			break;
		case F_TRUNCATE:
			flag_execute = false;
			tamanio = atoi(proxima_instruccion->parametro2);
			ejecutar_f_truncate(proxima_instruccion->parametro1, tamanio, contexto);
			log_info(logger,"Se esta ejecutando un F_TRUNCATE");
			break;
		case WAIT:
			flag_execute = false;
			ejecutar_wait(proxima_instruccion->parametro1, contexto);
			log_info(logger, "Se esta ejecutando un WAIT");
			break;
		case SIGNAL:
			flag_execute = false;
			ejecutar_signal(proxima_instruccion->parametro1, contexto);
			log_info(logger,"Se esta ejecutando un SIGNAL");
			break;
		case CREATE_SEGMENT:
			flag_execute = false;
			id_segmento = atoi(proxima_instruccion->parametro1);
			tamanio = atoi(proxima_instruccion->parametro2);
			ejecutar_create_segment(id_segmento, tamanio, contexto);
			log_info(logger,"Se esta ejecutando un CREATE_SEGMENT");
			break;
		case DELETE_SEGMENT:
			flag_execute = false;
			id_segmento = atoi(proxima_instruccion->parametro1);
			ejecutar_delete_segment(id_segmento, contexto);
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

int traducir_direccion(int dir_logica, t_contexto_ejecucion* contexto){

	int num_segmento = floor(dir_logica/TAM_MAX_SEGMENTO);

	int desplazamiento_segmento = dir_logica % TAM_MAX_SEGMENTO;

	int direccion_fisica = desplazamiento_segmento + num_segmento;	//TODO Chequear si la suma en el último párrafo de MMU es así

	if(direccion_fisica < TAM_MAX_SEGMENTO){
		return direccion_fisica;
	}else{
		send_contexto_ejecucion(contexto, socket_cliente);
		log_error(logger, "Error: Segmentation Fault (SEG_FAULT)");	//TODO Chequear como terminar la función en caso de que el kernel deba finalizar
		exit(1);
	}
}

void set_valor_registro(char* registro, char* valor){
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

		log_info(logger, "Se seteo el valor %s en registro %s", valor, registro);
}


void ejecutar_set(char* registro, char* valor){
	set_valor_registro(registro, valor);
	log_info(logger, "a mimir");
	usleep(RETARDO_INSTRUCCION * 1000);
}

//TODO Ejecutar_mov_in y ejecutar_mov_out, estos se envían para MEMORIA, NO KERNEL
void ejecutar_mov_in(char* registro, int dir_logica, t_contexto_ejecucion* contexto){
	int dir_fisica = traducir_direccion(dir_logica, contexto);
	send_leer_valor(dir_fisica, socket_cliente);
	char* valor_escrito_en_memoria = recv_valor(socket_cliente);
	set_valor_registro(registro, valor_escrito_en_memoria);
}

void ejecutar_mov_out(int dir_logica, char* registro){
	send_escribir_valor(registro, dir_logica, socket_cliente);
}

void ejecutar_io(int tiempo_io, t_contexto_ejecucion* contexto){
	send_contexto_ejecucion(contexto, socket_cliente);
	send_tiempo_io(tiempo_io, socket_cliente);
}

void ejecutar_f_open(char* nombre_archivo, t_contexto_ejecucion* contexto){
	send_contexto_ejecucion(contexto, socket_cliente);
	send_nombre_f_open(nombre_archivo, socket_cliente);
}

void ejecutar_f_close(char* nombre_archivo, t_contexto_ejecucion* contexto){
	send_contexto_ejecucion(contexto, socket_cliente);
	send_nombre_f_close(nombre_archivo, socket_cliente);
}

void ejecutar_f_seek(char* nombre_archivo, int posicion, t_contexto_ejecucion* contexto){
	send_contexto_ejecucion(contexto, socket_cliente);
	send_nombre_f_seek(nombre_archivo, socket_cliente);
}

void ejecutar_f_read(char* nombre_archivo, int dir_logica, int cantidad_bytes, t_contexto_ejecucion* contexto){
	send_contexto_ejecucion(contexto, socket_cliente);
	send_nombre_f_read(nombre_archivo, dir_logica, cantidad_bytes, socket_cliente);
}

void ejecutar_f_write(char* nombre_archivo, int dir_logica, int cantidad_bytes, t_contexto_ejecucion* contexto){
	send_contexto_ejecucion(contexto, socket_cliente);
	send_nombre_f_write(nombre_archivo, dir_logica, cantidad_bytes, socket_cliente);
}

void ejecutar_f_truncate(char* nombre_archivo, int tamanio, t_contexto_ejecucion* contexto){
	send_contexto_ejecucion(contexto, socket_cliente);
	send_nombre_f_truncate(nombre_archivo, tamanio, socket_cliente);
}

void ejecutar_wait(char* recurso, t_contexto_ejecucion* contexto){
	log_info(logger,"Se esta ejecutando un WAIT al recurso %s", recurso);
	send_contexto_ejecucion(contexto, socket_cliente);
	send_recurso_wait(recurso, socket_cliente);
}

void ejecutar_signal(char* recurso, t_contexto_ejecucion* contexto){
	send_contexto_ejecucion(contexto, socket_cliente);
	send_recurso_signal(recurso, socket_cliente);
}

void ejecutar_create_segment(int id_segmento, int tamanio, t_contexto_ejecucion* contexto){
	send_contexto_ejecucion(contexto, socket_cliente);
	send_create_segment(id_segmento, tamanio, socket_cliente);
}

void ejecutar_delete_segment(int id_segmento, t_contexto_ejecucion* contexto){
	send_contexto_ejecucion(contexto, socket_cliente);
	send_delete_segment(id_segmento, socket_cliente);
}

void ejecutar_yield(t_contexto_ejecucion* contexto){
	contexto->estado = READY;
	send_cambiar_estado(contexto, socket_cliente);
}

void ejecutar_exit(t_contexto_ejecucion* contexto){
	contexto->estado = FINISH_EXIT;
	contexto->motivo_exit = SUCCESS;
	send_cambiar_estado(contexto, socket_cliente);
}

void ejecutar_ciclo_de_instrucciones(t_contexto_ejecucion* contexto){
	while(flag_execute){
		fetch(contexto);
	}
}

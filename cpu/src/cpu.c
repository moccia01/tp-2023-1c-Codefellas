#include "../include/cpu.h"

int main(int argc, char **argv) {
	if (argc > 2) {
		return EXIT_FAILURE;
	}
	config = config_create(argv[1]);
	logger = log_create("cpu.log", "cpu_main", 1, LOG_LEVEL_INFO);
	logger_obligatorio = log_create("cpu.log", "cpu_obligatorio", 1, LOG_LEVEL_INFO);
	if(config == NULL){
		log_error(logger, "No se encontró el archivo :(");
		terminar_programa();
		exit(1);
	}
	leer_config();
	inicializar_variables();

	// Conecto CPU con memoria
	fd_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
	enviar_mensaje("Hola, soy CPU!", fd_memoria);

	// Inicio de servidor
	fd_cpu = iniciar_servidor(logger, IP, PUERTO);
	server_escuchar();

	terminar_programa();
	return 0;
}

// ------------------ INIT ------------------

void leer_config(){
	IP = config_get_string_value(config, "IP_ESCUCHA");
	PUERTO = config_get_string_value(config, "PUERTO_ESCUCHA");
	IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
	PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");
	RETARDO_INSTRUCCION = config_get_int_value(config, "RETARDO_INSTRUCCION");
	TAM_MAX_SEGMENTO = config_get_int_value(config, "TAM_MAX_SEGMENTO");
}

void terminar_programa(){
	log_destroy(logger);
	config_destroy(config);
	//registros_destroy(registros);
}

void inicializar_variables(){
	registros = inicializar_registro();
	flag_execute = true;
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
			log_info(logger, "Recibí el contexto del proceso %d y se inicia el ciclo de instruccion", contexto_de_ejecucion->pid);
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

	memcpy(registros->ax, "     ", 4);
	memcpy(registros->ax + 4, "\0", 1);

	memcpy(registros->bx, "     ", 4);
	memcpy(registros->bx + 4, "\0", 1);

	memcpy(registros->cx, "     ", 4);
	memcpy(registros->cx + 4, "\0", 1);

	memcpy(registros->dx, "     ", 4);
	memcpy(registros->dx + 4, "\0", 1);

	memcpy(registros->eax, "         ", 8);
	memcpy(registros->eax + 8, "\0", 1);

	memcpy(registros->ebx, "         ", 8);
	memcpy(registros->ebx + 8, "\0", 1);

	memcpy(registros->ecx, "         ", 8);
	memcpy(registros->ecx + 8, "\0", 1);

	memcpy(registros->edx, "         ", 8);
	memcpy(registros->edx + 8, "\0", 1);

	memcpy(registros->rax, "                 ", 16);
	memcpy(registros->rax + 16, "\0", 1);

	memcpy(registros->rbx, "                 ", 16);
	memcpy(registros->rbx + 16, "\0", 1);

	memcpy(registros->rcx, "                 ", 16);
	memcpy(registros->rcx + 16, "\0", 1);

	memcpy(registros->rdx, "                 ", 16);
	memcpy(registros->rdx + 16, "\0", 1);

//	registros->ax = "";
//	registros->bx = "";
//	registros->cx = "";
//	registros->dx = "";
//	registros->eax = "";
//	registros->ebx = "";
//	registros->ecx = "";
//	registros->edx = "";
//	registros->rax = "";
//	registros->rbx = "";
//	registros->rcx = "";
//	registros->rdx = "";

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

	log_info(logger_obligatorio, "PID: %d - Ejecutando: %s - %s %s %s", contexto->pid, instruccion_to_string(logger, proxima_instruccion->instruccion), proxima_instruccion->parametro1, proxima_instruccion->parametro2, proxima_instruccion->parametro3);
	//los logs son para testear e ir sabiendo lo que se va ejecutando
	switch(cod_instruccion){
		case SET:
			log_info(logger,"Se esta ejecutando un SET");
			ejecutar_set(proxima_instruccion->parametro1, proxima_instruccion->parametro2, contexto->registros);
			break;
		case MOV_IN:
			dir_logica = atoi(proxima_instruccion->parametro2);
			log_info(logger,"Se esta ejecutando un MOV_IN");
			ejecutar_mov_in(proxima_instruccion->parametro1, dir_logica, contexto);
			break;
		case MOV_OUT:
			dir_logica = atoi(proxima_instruccion->parametro1);
			log_info(logger,"Se esta ejecutando un MOV_OUT");
			ejecutar_mov_out(dir_logica, proxima_instruccion->parametro2, contexto);
			break;
		case IO:
			flag_execute = false;
			tiempo = atoi(proxima_instruccion->parametro1);
			log_info(logger,"Se esta ejecutando un I/O");
			ejecutar_io(tiempo, contexto);
			break;
		case F_OPEN:
			flag_execute = false;
			log_info(logger,"Se esta ejecutando un F_OPEN");
			ejecutar_f_open(proxima_instruccion->parametro1, contexto);
			break;
		case F_CLOSE:
			flag_execute = false;
			log_info(logger,"Se esta ejecutando un F_CLOSE");
			ejecutar_f_close(proxima_instruccion->parametro1, contexto);
			break;
		case F_SEEK:
			flag_execute = false;
			posicion = atoi(proxima_instruccion->parametro2);
			log_info(logger,"Se esta ejecutando un F_SEEK");
			ejecutar_f_seek(proxima_instruccion->parametro1, posicion, contexto);
			break;
		case F_READ:
			flag_execute = false;
			dir_logica = atoi(proxima_instruccion->parametro2);
			cantidad_bytes = atoi(proxima_instruccion->parametro3);
			log_info(logger,"Se esta ejecutando un F_READ");
			ejecutar_f_read(proxima_instruccion->parametro1, dir_logica, cantidad_bytes, contexto);
			break;
		case F_WRITE:
			flag_execute = false;
			dir_logica = atoi(proxima_instruccion->parametro2);
			cantidad_bytes = atoi(proxima_instruccion->parametro3);
			log_info(logger,"Se esta ejecutando un F_WRITE");
			ejecutar_f_write(proxima_instruccion->parametro1, dir_logica, cantidad_bytes, contexto);
			break;
		case F_TRUNCATE:
			flag_execute = false;
			tamanio = atoi(proxima_instruccion->parametro2);
			log_info(logger,"Se esta ejecutando un F_TRUNCATE");
			ejecutar_f_truncate(proxima_instruccion->parametro1, tamanio, contexto);
			break;
		case WAIT:
			flag_execute = false;
			log_info(logger, "El contexto tiene el valor de ax: %s", contexto->registros->ax);
			ejecutar_wait(proxima_instruccion->parametro1, contexto);
			break;
		case SIGNAL:
			flag_execute = false;
			log_info(logger,"Se esta ejecutando un SIGNAL");
			ejecutar_signal(proxima_instruccion->parametro1, contexto);
			break;
		case CREATE_SEGMENT:
			flag_execute = false;
			id_segmento = atoi(proxima_instruccion->parametro1);
			tamanio = atoi(proxima_instruccion->parametro2);
			log_info(logger,"Se esta ejecutando un CREATE_SEGMENT");
			ejecutar_create_segment(id_segmento, tamanio, contexto);
			break;
		case DELETE_SEGMENT:
			flag_execute = false;
			id_segmento = atoi(proxima_instruccion->parametro1);
			log_info(logger,"Se esta ejecutando un DELETE_SEGMENT");
			ejecutar_delete_segment(id_segmento, contexto);
			break;
		case YIELD:
			flag_execute = false;
			log_info(logger,"Se esta ejecutando un YIELD");
			ejecutar_yield(contexto);
			break;
		case EXIT:
			flag_execute = false;
			log_info(logger,"Se esta ejecutando un EXIT");
			ejecutar_exit(contexto);
			break;
		default:
			log_error(logger, "Instruccion no reconocida");
			return;
	}
}

t_segmento* obtener_segmento_de_tabla(t_list* tabla_de_segmentos, int num_segmento){
	log_info(logger, "la cantidad de segmentos de la tabla es: %d", list_size(tabla_de_segmentos));
	for(int i = 0; i < list_size(tabla_de_segmentos); i++){
		t_segmento* segmento_buscado = list_get(tabla_de_segmentos, i);
		log_info(logger, "el id del segmento en la posicion %d de la tabla es: %d", i, segmento_buscado->id);
		if(segmento_buscado->id == num_segmento){
			return segmento_buscado;
		}
	}
	return NULL;
}

t_traduccion_mmu* traducir_direccion(int dir_logica, t_contexto_ejecucion* contexto){
	t_traduccion_mmu* mmu = malloc(sizeof(t_traduccion_mmu));

	mmu->num_segmento = floor(dir_logica/TAM_MAX_SEGMENTO);
	mmu->desplazamiento_segmento = dir_logica % TAM_MAX_SEGMENTO;
	t_segmento* segmento_buscado = obtener_segmento_de_tabla(contexto->tabla_de_segmentos, mmu->num_segmento);
	mmu->tamanio = segmento_buscado->tamanio;
	mmu->dir_fisica = segmento_buscado->base + mmu->desplazamiento_segmento;
	log_info(logger, "la direccion fisica es %d = %d + %d", mmu->dir_fisica, segmento_buscado->base, mmu->desplazamiento_segmento);

	return mmu;
}

void set_valor_registro(char* registro, char* valor){
	strcat(valor, "\0");

	if(strcmp(registro, "AX") == 0){
		memcpy(registros->ax, valor, 5);
		log_info(logger, "Se seteo el valor %s en registro %s", registros->ax, "AX");
	}else if(strcmp(registro, "BX") == 0){
		memcpy(registros->bx, valor, 5);
		log_info(logger, "Se seteo el valor %s en registro %s", registros->bx, "BX");
	}else if(strcmp(registro, "CX") == 0){
		memcpy(registros->cx, valor, 5);
		log_info(logger, "Se seteo el valor %s en registro %s", registros->cx, "CX");
	}else if(strcmp(registro, "DX") == 0){
		memcpy(registros->dx, valor, 5);
		log_info(logger, "Se seteo el valor %s en registro %s", registros->dx, "DX");
	}else if(strcmp(registro, "EAX") == 0){
		memcpy(registros->eax, valor, 9);
		log_info(logger, "Se seteo el valor %s en registro %s", registros->eax, "EAX");
	}else if(strcmp(registro, "EBX") == 0){
		memcpy(registros->ebx, valor, 9);
		log_info(logger, "Se seteo el valor %s en registro %s", registros->ebx, "EBX");
	}else if(strcmp(registro, "ECX") == 0){
		memcpy(registros->ecx, valor, 9);
		log_info(logger, "Se seteo el valor %s en registro %s", registros->ecx, "ECX");
	}else if(strcmp(registro, "EDX") == 0){
		memcpy(registros->edx, valor, 9);
		log_info(logger, "Se seteo el valor %s en registro %s", registros->edx, "EDX");
	}else if(strcmp(registro, "RAX") == 0){
		memcpy(registros->rax, valor, 17);
		log_info(logger, "Se seteo el valor %s en registro %s", registros->rax, "RAX");
	}else if(strcmp(registro, "RBX") == 0){
		memcpy(registros->rbx, valor, 17);
		log_info(logger, "Se seteo el valor %s en registro %s", registros->rbx, "RBX");
	}else if(strcmp(registro, "RCX") == 0){
		memcpy(registros->rcx, valor, 17);
		log_info(logger, "Se seteo el valor %s en registro %s", registros->rcx, "RCX");
	}else if(strcmp(registro, "RDX") == 0){
		memcpy(registros->rdx, valor, 17);
		log_info(logger, "Se seteo el valor %s en registro %s", registros->rdx, "RDX");
	}

}

int obtener_tamanio_registro(char* registro){

	if((strcmp(registro, "AX") == 0) || (strcmp(registro, "BX") == 0) || (strcmp(registro, "CX") == 0) || (strcmp(registro, "DX") == 0)){
		return 4;
	}
	else if((strcmp(registro, "EAX") == 0) || (strcmp(registro, "EBX") == 0) || (strcmp(registro, "ECX") == 0) || (strcmp(registro, "EDX") == 0)){
		return 8;
	}
	else if((strcmp(registro, "RAX") == 0) || (strcmp(registro, "RBX") == 0) || (strcmp(registro, "RCX") == 0) || (strcmp(registro, "RDX") == 0)){
		return 16;
	}else{
		log_info(logger, "Registro no reconocido: %s", registro);
		return -1;
	}
}

char* leer_valor_registro(char* registro){
	char* valor;

	if(strcmp(registro, "AX") == 0){
		valor = malloc(4);
		memcpy(valor, registros->ax, 4);
		return valor;
	}else if(strcmp(registro, "BX") == 0){
		valor = malloc(4);
		memcpy(valor, registros->bx, 4);
		return valor;
	}else if(strcmp(registro, "CX") == 0){
		valor = malloc(4);
		memcpy(valor, registros->cx, 4);
		return valor;
	}else if(strcmp(registro, "DX") == 0){
		valor = malloc(4);
		memcpy(valor, registros->dx, 4);
		return valor;
	}else if(strcmp(registro, "EAX") == 0){
		valor = malloc(8);
		memcpy(valor, registros->eax, 8);
		return valor;
	}else if(strcmp(registro, "EBX") == 0){
		valor = malloc(8);
		memcpy(valor, registros->ebx, 8);
		return valor;
	}else if(strcmp(registro, "ECX") == 0){
		valor = malloc(8);
		memcpy(valor, registros->ecx, 8);
		return valor;
	}else if(strcmp(registro, "EDX") == 0){
		valor = malloc(8);
		memcpy(valor, registros->edx, 8);
		return valor;
	}else if(strcmp(registro, "RAX") == 0){
		valor = malloc(16);
		memcpy(valor, registros->rax, 16);
		return valor;
	}else if(strcmp(registro, "RBX") == 0){
		valor = malloc(16);
		memcpy(valor, registros->rbx, 16);
		return valor;
	}else if(strcmp(registro, "RCX") == 0){
		valor = malloc(16);
		memcpy(valor, registros->rcx, 16);
		return valor;
	}else if(strcmp(registro, "RDX") == 0){
		valor = malloc(16);
		memcpy(valor, registros->rdx, 16);
		return valor;
	}else{
		log_info(logger, "No es encontró el registro pasado: %s", registro);
		return NULL;
	}
}

void manejar_seg_fault(t_contexto_ejecucion* contexto, t_traduccion_mmu* mmu, int tamanio){
	contexto->motivo_exit = SEG_FAULT;
	log_info(logger_obligatorio, "PID %d - Error SEG_FAULT - Segmento: %d - Offset: %d - Tamaño: %d", contexto->pid, mmu->num_segmento, mmu->desplazamiento_segmento, tamanio);
	send_contexto_ejecucion(contexto, socket_cliente);
	send_cambiar_estado(FINISH_ERROR, socket_cliente);
	flag_execute = false;
}

void ejecutar_set(char* registro, char* valor, t_registros* registros_contexto){
	set_valor_registro(registro, valor);
	actualizar_registros_contexto(registros_contexto);
	log_info(logger, "A mimir");
	usleep(RETARDO_INSTRUCCION * 1000);
}

void ejecutar_mov_in(char* registro, int dir_logica, t_contexto_ejecucion* contexto){

	t_traduccion_mmu* mmu = traducir_direccion(dir_logica, contexto);
	int tamanio_a_leer = obtener_tamanio_registro(registro);

	if(tamanio_a_leer + mmu->desplazamiento_segmento > mmu->tamanio){
		manejar_seg_fault(contexto, mmu, tamanio_a_leer);
	}else{
		send_leer_valor_cpu(mmu->dir_fisica, tamanio_a_leer, contexto->pid, fd_memoria);
		char* valor_leido_en_memoria = recv_valor_leido_cpu(fd_memoria);
		char* valor_log = malloc(tamanio_a_leer + 1);
		memcpy(valor_log, valor_leido_en_memoria, tamanio_a_leer);
		memcpy(valor_log + tamanio_a_leer, "\0", 1);
		log_info(logger_obligatorio, "PID: %d - Acción: LEER - Segmento: %d - Dirección Física: %d - Valor: %s", contexto->pid, mmu->num_segmento, mmu->dir_fisica, valor_log);
		set_valor_registro(registro, valor_log);
		// TODO free(valor_leido_memoria);
	}
}

void ejecutar_mov_out(int dir_logica, char* registro, t_contexto_ejecucion* contexto){

	t_traduccion_mmu* mmu = traducir_direccion(dir_logica, contexto);
	int tamanio_a_escribir = obtener_tamanio_registro(registro);

	if(tamanio_a_escribir + mmu->desplazamiento_segmento > mmu->tamanio){
		manejar_seg_fault(contexto, mmu, tamanio_a_escribir);
	}else{
		char* valor_escrito_en_memoria = leer_valor_registro(registro);
		char* valor_log = malloc(tamanio_a_escribir + 1);
		memcpy(valor_log, valor_escrito_en_memoria, tamanio_a_escribir);
		memcpy(valor_log + tamanio_a_escribir, "\0", 1);
		log_info(logger_obligatorio, "PID: %d - Acción: ESCRIBIR - Segmento: %d - Dirección Física: %d - Valor: %s", contexto->pid, mmu->num_segmento, mmu->dir_fisica, valor_log);
		send_escribir_valor_cpu(valor_escrito_en_memoria, mmu->dir_fisica, tamanio_a_escribir, contexto->pid, fd_memoria);
		// TODO free(valor_escrito_en_memoria);
		recv_fin_escritura(fd_memoria);
	}
}

void ejecutar_io(int tiempo_io, t_contexto_ejecucion* contexto){
	send_contexto_ejecucion(contexto, socket_cliente);
	send_tiempo_io(tiempo_io, socket_cliente);
}

void ejecutar_f_open(char* nombre_archivo, t_contexto_ejecucion* contexto){
	log_info(logger,"Me llego este nombre: %s", nombre_archivo);
	char* nombre = malloc(strlen(nombre_archivo) + 1);
	strcpy(nombre, nombre_archivo);
	send_contexto_ejecucion(contexto, socket_cliente);
	send_manejar_f_open(nombre, socket_cliente);
	free(nombre);
}

void ejecutar_f_close(char* nombre_archivo, t_contexto_ejecucion* contexto){
	char* nombre = malloc(strlen(nombre_archivo) + 1);
	strcpy(nombre, nombre_archivo);
	send_contexto_ejecucion(contexto, socket_cliente);
	send_manejar_f_close(nombre, socket_cliente);
	free(nombre);
}

void ejecutar_f_seek(char* nombre_archivo, int posicion, t_contexto_ejecucion* contexto){
	char* nombre = malloc(strlen(nombre_archivo) + 1);
	strcpy(nombre, nombre_archivo);
	send_contexto_ejecucion(contexto, socket_cliente);
	send_manejar_f_seek(nombre, posicion, socket_cliente);
	free(nombre);
}

void ejecutar_f_read(char* nombre_archivo, int dir_logica, int cantidad_bytes, t_contexto_ejecucion* contexto){
	t_traduccion_mmu* mmu = traducir_direccion(dir_logica, contexto);
	char* nombre = malloc(strlen(nombre_archivo) + 1);
	strcpy(nombre, nombre_archivo);
	send_contexto_ejecucion(contexto, socket_cliente);
	send_manejar_f_read(nombre, mmu->dir_fisica, cantidad_bytes, socket_cliente);
	free(nombre);
}

void ejecutar_f_write(char* nombre_archivo, int dir_logica, int cantidad_bytes, t_contexto_ejecucion* contexto){
	t_traduccion_mmu* mmu = traducir_direccion(dir_logica, contexto);
	char* nombre = malloc(strlen(nombre_archivo) + 1);
	strcpy(nombre, nombre_archivo);
	send_contexto_ejecucion(contexto, socket_cliente);
	send_manejar_f_write(nombre, mmu->dir_fisica, cantidad_bytes, socket_cliente);
	free(nombre);
}

void ejecutar_f_truncate(char* nombre_archivo, int tamanio, t_contexto_ejecucion* contexto){
	char* nombre = malloc(strlen(nombre_archivo) + 1);
	strcpy(nombre, nombre_archivo);
	send_contexto_ejecucion(contexto, socket_cliente);
	send_manejar_f_truncate(nombre, tamanio, socket_cliente);
	free(nombre);
}

void ejecutar_wait(char* recurso, t_contexto_ejecucion* contexto){
	log_info(logger,"Se esta ejecutando un WAIT al recurso %s", recurso);
	char* r = malloc(strlen(recurso) + 1);
	strcpy(r, recurso);
	send_contexto_ejecucion(contexto, socket_cliente);
	send_recurso_wait(r, socket_cliente);
	free(r);
}

void ejecutar_signal(char* recurso, t_contexto_ejecucion* contexto){
	char* r = malloc(strlen(recurso) + 1);
	strcpy(r, recurso);
	send_contexto_ejecucion(contexto, socket_cliente);
	send_recurso_signal(r, socket_cliente);
	free(r);
}

void ejecutar_create_segment(int id_segmento, int tamanio, t_contexto_ejecucion* contexto){
	send_contexto_ejecucion(contexto, socket_cliente);
	send_create_segment(contexto->pid, id_segmento, tamanio, socket_cliente);
}

void ejecutar_delete_segment(int id_segmento, t_contexto_ejecucion* contexto){
	send_contexto_ejecucion(contexto, socket_cliente);
	send_delete_segment(contexto->pid, id_segmento, socket_cliente);
}

void ejecutar_yield(t_contexto_ejecucion* contexto){
	send_contexto_ejecucion(contexto, socket_cliente);
	send_cambiar_estado(READY, socket_cliente);
}

void ejecutar_exit(t_contexto_ejecucion* contexto){
	send_contexto_ejecucion(contexto, socket_cliente);
	send_cambiar_estado(FINISH_EXIT, socket_cliente);
}

void ejecutar_ciclo_de_instrucciones(t_contexto_ejecucion* contexto){
	while(flag_execute){
		fetch(contexto);
	}
}

void actualizar_registros_contexto(t_registros* registros_contexto){
	strcpy(registros_contexto->ax, registros->ax);
//	log_info(logger, "Ahora el contexto tiene el valor de ax: %s", registros_contexto->ax);
	strcpy(registros_contexto->bx, registros->bx);
//	log_info(logger, "Ahora el contexto tiene el valor de bx: %s", registros_contexto->bx);
	strcpy(registros_contexto->cx, registros->cx);
//	log_info(logger, "Ahora el contexto tiene el valor de cx: %s", registros_contexto->cx);
	strcpy(registros_contexto->dx, registros->dx);
//	log_info(logger, "Ahora el contexto tiene el valor de dx: %s", registros_contexto->dx);
	strcpy(registros_contexto->eax, registros->eax);
//	log_info(logger, "Ahora el contexto tiene el valor de eax: %s", registros_contexto->eax);
	strcpy(registros_contexto->ebx, registros->ebx);
//	log_info(logger, "Ahora el contexto tiene el valor de ebx: %s", registros_contexto->ebx);
	strcpy(registros_contexto->ecx, registros->ecx);
//	log_info(logger, "Ahora el contexto tiene el valor de ecx: %s", registros_contexto->ecx);
	strcpy(registros_contexto->edx, registros->edx);
//	log_info(logger, "Ahora el contexto tiene el valor de edx: %s", registros_contexto->edx);
	strcpy(registros_contexto->rax, registros->rax);
//	log_info(logger, "Ahora el contexto tiene el valor de rax: %s", registros_contexto->rax);
	strcpy(registros_contexto->rbx, registros->rbx);
//	log_info(logger, "Ahora el contexto tiene el valor de rbx: %s", registros_contexto->rbx);
	strcpy(registros_contexto->rcx, registros->rcx);
//	log_info(logger, "Ahora el contexto tiene el valor de rcx: %s", registros_contexto->rcx);
	strcpy(registros_contexto->rdx, registros->rdx);
//	log_info(logger, "Ahora el contexto tiene el valor de rdx: %s", registros_contexto->rdx);

}

#include "../include/protocolo.h"

void* serializar_paquete(t_paquete* paquete, int bytes){
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

void enviar_mensaje(char* mensaje, int socket_cliente){
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

void eliminar_paquete(t_paquete* paquete){
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

int recibir_operacion(int socket_cliente){
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void* recibir_buffer(int* size, int socket_cliente){
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(t_log* logger, int socket_cliente){
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje: %s", buffer);
	free(buffer);
}

t_list* recibir_paquete(int socket_cliente){
	int size;
	int desplazamiento = 0;
	void* buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size){
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}

void crear_buffer(t_paquete* paquete){
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

t_paquete* crear_paquete(op_code codigo_op){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = codigo_op;
	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio){
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete* paquete, int socket_cliente){
	int bytes = paquete->buffer->size + sizeof(int) + sizeof(op_code);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void empaquetar_instrucciones(t_paquete* instrucciones_a_mandar, t_list* lista_de_instrucciones){
	int cantidad_instrucciones = list_size(lista_de_instrucciones);
	agregar_a_paquete(instrucciones_a_mandar, &(cantidad_instrucciones), sizeof(int));
	for(int i=0; i<cantidad_instrucciones; i++){
		t_instruccion* instruccion = list_get(lista_de_instrucciones, i);
		agregar_a_paquete(instrucciones_a_mandar, &(instruccion->instruccion), sizeof(cod_instruccion));
		agregar_a_paquete(instrucciones_a_mandar, instruccion->parametro1, strlen(instruccion->parametro1) + 1);
		agregar_a_paquete(instrucciones_a_mandar, instruccion->parametro2, strlen(instruccion->parametro2) + 1);
		agregar_a_paquete(instrucciones_a_mandar, instruccion->parametro3, strlen(instruccion->parametro3) + 1);
	}
}

t_list* desempaquetar_instrucciones(t_list* paquete, int comienzo){
	t_list* instrucciones = list_create();
	int* cantidad_instrucciones = list_get(paquete, comienzo);
	int i = comienzo + 1;

	while(i - comienzo - 1< (*cantidad_instrucciones* 4)){
		t_instruccion* instruccion = malloc(sizeof(t_instruccion));
		cod_instruccion* cod = list_get(paquete, i);
		instruccion->instruccion = *cod;
		free(cod);
		i++;

		char* param1 = list_get(paquete, i);
		instruccion->parametro1 = malloc(strlen(param1));
		strcpy(instruccion->parametro1, param1);
		free(param1);
		i++;

		char* param2 = list_get(paquete, i);
		instruccion->parametro2 = malloc(strlen(param2));
		strcpy(instruccion->parametro2, param2);
		free(param2);
		i++;

		char* param3 = list_get(paquete, i);
		instruccion->parametro3 = malloc(strlen(param3));
		strcpy(instruccion->parametro3, param3);
		free(param3);
		i++;

		list_add(instrucciones, instruccion);
	}
	free(cantidad_instrucciones);
	return instrucciones;
}

void send_instrucciones(int fd_modulo,t_list* lista_de_instrucciones){
	t_paquete* instrucciones_a_mandar = crear_paquete(INSTRUCCIONES_CONSOLA);
	empaquetar_instrucciones(instrucciones_a_mandar, lista_de_instrucciones);
	enviar_paquete(instrucciones_a_mandar, fd_modulo);
	eliminar_paquete(instrucciones_a_mandar);
}

t_list* recv_instrucciones(t_log* logger, int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	t_list* instrucciones = desempaquetar_instrucciones(paquete, 0);
	list_destroy(paquete);
	log_info(logger, "Se recibió una lista de instrucciones.");
	return instrucciones;
}

void instruccion_destroyer(t_instruccion* instruccion){
	free(instruccion->parametro1);
	free(instruccion->parametro2);
	free(instruccion->parametro3);
	free(instruccion);
	instruccion = NULL;
}

void empaquetar_tabla_segmentos(t_paquete* paquete, t_list* tabla_segmentos){
	int cantidad_segmentos = list_size(tabla_segmentos);
	agregar_a_paquete(paquete, &(cantidad_segmentos), sizeof(int));
	for(int i = 0; i < cantidad_segmentos; i++){
		t_segmento* segmento = list_get(tabla_segmentos, i);
		empaquetar_segmento(paquete, segmento);
	}
}

t_list* desempaquetar_tabla_segmentos(t_list* paquete, int comienzo){
	t_list* tabla_segmentos = list_create();
	int* cantidad_segmentos = list_get(paquete, comienzo);
	int i = comienzo + 1;
	while (i - comienzo + 1 < *(cantidad_segmentos) * 3){
		t_segmento* segmento = desempaquetar_segmento(paquete, i);
		i += 3;
		list_add(tabla_segmentos, segmento);
	}
	free(cantidad_segmentos);
	return tabla_segmentos;
}

void empaquetar_registro_contexto(t_paquete* paquete, t_registros* registros){


	if(registros->ax != NULL){
		agregar_a_paquete(paquete, registros->ax, strlen(registros->ax) + 1);
	}
	if(registros->bx != NULL){
		agregar_a_paquete(paquete, registros->bx, strlen(registros->bx) + 1);
	}
	if(registros->cx != NULL){
		agregar_a_paquete(paquete, registros->cx, strlen(registros->cx) + 1);
	}
	if(registros->dx != NULL){
		agregar_a_paquete(paquete, registros->dx, strlen(registros->dx) + 1);
	}
	if(registros->eax != NULL){
		agregar_a_paquete(paquete, registros->eax, strlen(registros->eax) + 1);
	}
	if(registros->ebx != NULL){
		agregar_a_paquete(paquete, registros->ebx, strlen(registros->ebx) + 1);
	}
	if(registros->ecx != NULL){
		agregar_a_paquete(paquete, registros->ecx, strlen(registros->ecx) + 1);
	}
	if(registros->edx != NULL){
		agregar_a_paquete(paquete, registros->edx, strlen(registros->edx) + 1);
	}
	if(registros->rax != NULL){
		agregar_a_paquete(paquete, registros->rax, strlen(registros->rax) + 1);
	}
	if(registros->rbx != NULL){
		agregar_a_paquete(paquete, registros->rbx, strlen(registros->rbx) + 1);
	}
	if(registros->rcx != NULL){
		agregar_a_paquete(paquete, registros->rcx, strlen(registros->rcx) + 1);
	}
	if(registros->rdx != NULL){
		agregar_a_paquete(paquete, registros->rdx, strlen(registros->rdx) + 1);
	}
}

t_registros* desempaquetar_registros(t_list* paquete, int comienzo){
	t_registros* registro_contexto = malloc(sizeof(t_registros));

	char* ax = list_get(paquete,comienzo);
	registro_contexto->ax = malloc(sizeof(ax));
	strcpy(registro_contexto->ax, ax);
	free(ax);

	char* bx = list_get(paquete,comienzo + 1);
	registro_contexto->bx = malloc(sizeof(bx));
	strcpy(registro_contexto->bx, bx);
	free(bx);

	char* cx = list_get(paquete,comienzo + 2);
	registro_contexto->cx = malloc(sizeof(cx));
	strcpy(registro_contexto->cx, cx);
	free(cx);

	char* dx = list_get(paquete,comienzo + 3);
	registro_contexto->dx = malloc(sizeof(dx));
	strcpy(registro_contexto->dx, dx);
	free(dx);

	char* eax = list_get(paquete,comienzo + 4);
	registro_contexto->eax = malloc(sizeof(eax));
	strcpy(registro_contexto->eax, eax);
	free(eax);

	char* ebx = list_get(paquete,comienzo + 5);
	registro_contexto->ebx = malloc(sizeof(ebx));
	strcpy(registro_contexto->ebx, ebx);
	free(ebx);

	char* ecx = list_get(paquete,comienzo + 6);
	registro_contexto->ecx = malloc(sizeof(ecx));
	strcpy(registro_contexto->ecx, ecx);
	free(ecx);

	char* edx = list_get(paquete,comienzo + 7);
	registro_contexto->edx = malloc(sizeof(edx));
	strcpy(registro_contexto->edx, edx);
	free(edx);

	char* rax = list_get(paquete,comienzo + 8);
	registro_contexto->rax = malloc(sizeof(rax));
	strcpy(registro_contexto->rax, rax);
	free(rax);

	char* rbx = list_get(paquete,comienzo + 9);
	registro_contexto->rbx = malloc(sizeof(rbx));
	strcpy(registro_contexto->rbx, rbx);
	free(rbx);

	char* rcx = list_get(paquete,comienzo + 10);
	registro_contexto->rcx = malloc(sizeof(rcx));
	strcpy(registro_contexto->rcx, rcx);
	free(rcx);

	char* rdx = list_get(paquete,comienzo + 11);
	registro_contexto->rdx = malloc(sizeof(rdx));
	strcpy(registro_contexto->rdx, rdx);
	free(rdx);

	return registro_contexto;
}

void empaquetar_contexto_ejecucion(t_paquete* paquete, t_contexto_ejecucion* contexto){
	agregar_a_paquete(paquete, &(contexto->pid), sizeof(int));
	agregar_a_paquete(paquete, &(contexto->program_counter), sizeof(int));
	agregar_a_paquete(paquete, &(contexto->estado), sizeof(estado_proceso));
	agregar_a_paquete(paquete, &(contexto->motivo_exit), sizeof(motivo_exit));
	agregar_a_paquete(paquete, &(contexto->motivo_block), sizeof(motivo_block));
	empaquetar_registro_contexto(paquete, contexto->registros);
	empaquetar_instrucciones(paquete, contexto->instrucciones);
	empaquetar_tabla_segmentos(paquete, contexto->tabla_de_segmentos);
}

void empaquetar_segmento(t_paquete* paquete, t_segmento* segmento){
	agregar_a_paquete(paquete, &(segmento->base), sizeof(int));
	agregar_a_paquete(paquete, &(segmento->id), sizeof(int));
	agregar_a_paquete(paquete, &(segmento->tamanio), sizeof(int));
}

t_segmento* desempaquetar_segmento(t_list* paquete, int comienzo){
	t_segmento* segmento = malloc(sizeof(t_segmento));

	int* base = list_get(paquete, comienzo);
	segmento->base = *base;
	free(base);

	int* id = list_get(paquete, comienzo + 1);
	segmento->id = *id;
	free(id);

	int* tamanio = list_get(paquete, comienzo + 2);
	segmento->tamanio = *tamanio;
	free(tamanio);

	return segmento;
}

t_contexto_ejecucion* desempaquetar_contexto_ejecucion(t_list* paquete){
	t_contexto_ejecucion* contexto = malloc(sizeof(t_contexto_ejecucion));

	int* pid = list_get(paquete, 0);
	contexto->pid = *pid;
	free(pid);

	int* program_counter = list_get(paquete, 1);
	contexto->program_counter = *program_counter;
	free(program_counter);

	estado_proceso* estado =  list_get(paquete, 2);
	contexto->estado = *estado;
	free(estado);

	motivo_exit* motivo_exit = list_get(paquete, 3);
	contexto->motivo_exit = *motivo_exit;
	free(motivo_exit);

	motivo_block* motivo_block = list_get(paquete, 4);
	contexto->motivo_block = *motivo_block;
	free(motivo_block);

	int comienzo_registros = 5;
	t_registros* registro_contexto = desempaquetar_registros(paquete, comienzo_registros);
	contexto->registros = registro_contexto;

	int comienzo_instrucciones = comienzo_registros + 12;
	t_list* instrucciones = desempaquetar_instrucciones(paquete, comienzo_instrucciones);
	contexto->instrucciones = instrucciones;
	int cantidad_instrucciones = list_size(instrucciones);

	int comienzo_segmentos = comienzo_instrucciones + (cantidad_instrucciones * 4) + 1;
	t_list* tabla_segmentos = desempaquetar_tabla_segmentos(paquete, comienzo_segmentos);
	contexto->tabla_de_segmentos = tabla_segmentos;

	return contexto;
}

void send_contexto_ejecucion(t_contexto_ejecucion* contexto, int fd_modulo){
	t_paquete* paquete = crear_paquete(CONTEXTO_EJECUCION);
	empaquetar_contexto_ejecucion(paquete, contexto);
	enviar_paquete(paquete, fd_modulo);
	contexto_destroyer(contexto);
	eliminar_paquete(paquete);
}

t_contexto_ejecucion* recv_contexto_ejecucion(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	t_contexto_ejecucion* contexto_recibido = desempaquetar_contexto_ejecucion(paquete);
	list_destroy(paquete);
	return contexto_recibido;
}

void contexto_destroyer(t_contexto_ejecucion* contexto){
	lista_instrucciones_destroy(contexto->instrucciones);
	list_destroy_and_destroy_elements(contexto->tabla_de_segmentos, (void*) segmento_destroy);
	registros_destroy(contexto->registros);
	free(contexto);
	contexto = NULL;
}

void segmento_destroy(t_segmento* segmento){
	free(segmento);
	segmento = NULL;
}

void registros_destroy(t_registros* registros){
//	free(registros->ax);
//	free(registros->bx);
//	free(registros->cx);
//	free(registros->dx);
//	free(registros->eax);
//	free(registros->ebx);
//	free(registros->ecx);
//	free(registros->edx);
//	free(registros->rax);
//	free(registros->rbx);
//	free(registros->rcx);
//	free(registros->rdx);
	free(registros);
//	registros = NULL;
}

void lista_instrucciones_destroy(t_list* lista){
	while(!list_is_empty(lista)){
		t_instruccion* instrucion = list_remove(lista, 0);
		instruccion_destroyer(instrucion);
	}
	list_destroy(lista);
}

void send_cambiar_estado(estado_proceso estado, int fd_modulo){
	t_paquete* paquete = crear_paquete(CAMBIAR_ESTADO);
	agregar_a_paquete(paquete, &estado, sizeof(estado_proceso));
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

estado_proceso recv_cambiar_estado(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	estado_proceso* estado = list_get(paquete, 0);
	estado_proceso ret = *estado;
	free(estado);
	list_destroy(paquete);
	return ret;
}

void send_tiempo_io(int tiempo_io, int fd_modulo){
	t_paquete* paquete = crear_paquete(MANEJAR_IO);
	agregar_a_paquete(paquete, &(tiempo_io), sizeof(int));
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

void send_recurso_wait(char* recurso, int fd_modulo){
	t_paquete* paquete = crear_paquete(MANEJAR_WAIT);
	agregar_a_paquete(paquete, recurso, strlen(recurso) + 1);
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

void send_recurso_signal(char* recurso, int fd_modulo){
	t_paquete* paquete = crear_paquete(MANEJAR_SIGNAL);
	agregar_a_paquete(paquete, recurso, strlen(recurso) + 1);
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

int recv_tiempo_io(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	int* tiempo = list_get(paquete, 0);
	int ret = *tiempo;
	free(tiempo);
	list_destroy(paquete);
	return ret;
}

char* recv_recurso(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	char* recurso = list_get(paquete, 0);
	list_destroy(paquete);

	return recurso;
}

void send_manejar_f_open(char* nombre_archivo, int fd_modulo){
	t_paquete* paquete = crear_paquete(MANEJAR_F_OPEN);
	agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);
	enviar_paquete(paquete, fd_modulo);
}

void send_manejar_f_close(char* nombre_archivo, int fd_modulo){
	t_paquete* paquete = crear_paquete(MANEJAR_F_CLOSE);
	agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);
	enviar_paquete(paquete, fd_modulo);
}

void send_manejar_f_seek(char* nombre_archivo, int posicion, int fd_modulo){
	t_paquete* paquete = crear_paquete(MANEJAR_F_SEEK);
	agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);
	agregar_a_paquete(paquete, &posicion, sizeof(int));
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

void send_manejar_f_read(char* nombre_archivo, int dir_fisica, int cantidad_bytes, int fd_modulo){
	t_paquete* paquete = crear_paquete(MANEJAR_F_READ);
	agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);
	agregar_a_paquete(paquete, &(dir_fisica), sizeof(int));
	agregar_a_paquete(paquete, &(cantidad_bytes), sizeof(int));
	enviar_paquete(paquete, fd_modulo);
}

void send_manejar_f_write(char* nombre_archivo, int dir_fisica, int cantidad_bytes, int fd_modulo){
	t_paquete* paquete = crear_paquete(MANEJAR_F_WRITE);
	agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);
	agregar_a_paquete(paquete, &(dir_fisica), sizeof(int));
	agregar_a_paquete(paquete, &(cantidad_bytes), sizeof(int));
	enviar_paquete(paquete, fd_modulo);
}

void send_manejar_f_truncate(char* nombre_archivo, int tamanio, int fd_modulo){
	t_paquete* paquete = crear_paquete(MANEJAR_F_TRUNCATE);
	agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);
	agregar_a_paquete(paquete, &(tamanio), sizeof(int));
	enviar_paquete(paquete, fd_modulo);
}

void send_create_segment(int pid, int id_segmento, int tamanio, int fd_modulo){
	t_paquete* paquete = crear_paquete(MANEJAR_CREATE_SEGMENT);
	agregar_a_paquete(paquete, &(id_segmento), sizeof(int));
	agregar_a_paquete(paquete, &(tamanio), sizeof(int));
	agregar_a_paquete(paquete, &(pid), sizeof(int));
	enviar_paquete(paquete, fd_modulo);
}

t_list* recv_create_segment(int fd_modulo){
	return recibir_paquete(fd_modulo);
}

void send_segment_response(t_segment_response resp, int fd_modulo){
	t_paquete* paquete = crear_paquete(SEGMENT_RESPONSE);
	agregar_a_paquete(paquete, &resp, sizeof(t_segment_response));
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

t_segment_response recv_segment_response(int fd_modulo){
	op_code cop = recibir_operacion(fd_modulo);
	if(cop != SEGMENT_RESPONSE){
		return -1;
	}
	t_list* paquete = recibir_paquete(fd_modulo);
	t_segment_response* respuesta = list_get(paquete, 0);
	return *respuesta;
}

void send_delete_segment(int pid, int id_segmento, int fd_modulo){
	t_paquete* paquete = crear_paquete(MANEJAR_DELETE_SEGMENT);
	agregar_a_paquete(paquete, &(id_segmento), sizeof(int));
	agregar_a_paquete(paquete, &(pid), sizeof(int));
	enviar_paquete(paquete, fd_modulo);
}

t_list* recv_delete_segment(int fd_modulo){
	return recibir_paquete(fd_modulo);
}

void send_tabla_segmentos(t_list* tabla_segmentos, int fd_modulo){
	t_paquete* paquete = crear_paquete(TABLA_SEGMENTOS);
	empaquetar_tabla_segmentos(paquete, tabla_segmentos);
	enviar_paquete(paquete, fd_modulo);
}

t_list* recv_tabla_segmentos(int fd_modulo){
	op_code cop = recibir_operacion(fd_modulo);
	if(cop != TABLA_SEGMENTOS){
		return NULL;
	}
	t_list* paquete = recibir_paquete(fd_modulo);
	t_list* tabla_segmentos = desempaquetar_tabla_segmentos(paquete, 0);
	return tabla_segmentos;
}

void send_leer_valor(int dir_fisica, int tamanio_a_leer, int fd_modulo){
	t_paquete* paquete = crear_paquete(PEDIDO_LECTURA);
	agregar_a_paquete(paquete, &(dir_fisica), sizeof(int));
	agregar_a_paquete(paquete, &(tamanio_a_leer), sizeof(int));
	enviar_paquete(paquete, fd_modulo);
}

t_list* recv_leer_valor(int fd_modulo){
	return recibir_paquete(fd_modulo);
}

char* recv_valor(int fd_modulo){
	op_code cop = recibir_operacion(fd_modulo);
	if(cop != PEDIDO_LECTURA){
		return NULL;
	}
	t_list* paquete = recibir_paquete(fd_modulo);
	char* valor = list_get(paquete, 0);
	return valor;
}

void send_valor_leido(char* valor, int fd_modulo){
	t_paquete* paquete = crear_paquete(PEDIDO_LECTURA);
	agregar_a_paquete(paquete, valor, strlen(valor) + 1);
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

void send_escribir_valor(char* valor, int dir_fisica, int fd_modulo){
	t_paquete* paquete = crear_paquete(PEDIDO_ESCRITURA);
	agregar_a_paquete(paquete, valor, strlen(valor) + 1);
	agregar_a_paquete(paquete, &(dir_fisica), sizeof(int));
	enviar_paquete(paquete, fd_modulo);
}

t_list* recv_escribir_valor(int fd_modulo){
	return recibir_paquete(fd_modulo);
}

void send_inicializar_proceso(int pid, int fd_modulo){
	t_paquete* paquete = crear_paquete(INICIALIZAR_PROCESO);
	agregar_a_paquete(paquete, &pid, sizeof(int));
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

int recv_inicializar_proceso(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	int* pid = list_get(paquete, 0);
	list_destroy(paquete);
	return *pid;
}

void send_proceso_inicializado(t_list* tabla_segmentos, int fd_modulo){
	t_paquete* paquete = crear_paquete(INICIALIZAR_PROCESO);
	empaquetar_tabla_segmentos(paquete, tabla_segmentos);
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

t_list* recv_proceso_inicializado(int fd_modulo){
	op_code cop = recibir_operacion(fd_modulo);
	if(cop != INICIALIZAR_PROCESO){
		return NULL;
	}
	t_list* paquete = recibir_paquete(fd_modulo);
	t_list* tabla_segmentos = desempaquetar_tabla_segmentos(paquete, 0);
	list_destroy(paquete);
	return tabla_segmentos;
}

void send_terminar_proceso(int pid, int fd_modulo){
	t_paquete* paquete = crear_paquete(FINALIZAR_PROCESO);
	agregar_a_paquete(paquete, &pid, sizeof(int));
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

int recv_terminar_proceso(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	int* pid = list_get(paquete, 0);
	list_destroy(paquete);
	return *pid;
}

void send_base_segmento(int base, int fd_modulo){
	t_paquete* paquete = crear_paquete(BASE_SEGMENTO);
	agregar_a_paquete(paquete, &base, sizeof(int));
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

int recv_base_segmento(int fd_modulo){
	op_code cop = recibir_operacion(fd_modulo);
	if(cop != BASE_SEGMENTO){
		return -1;
	}
	t_list* paquete = recibir_paquete(fd_modulo);
	int* base = list_get(paquete, 0);
	list_destroy(paquete);
	return *base;
}

char* recv_manejo_f_open(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	char* nombre_archivo = list_get(paquete, 0);
	list_destroy(paquete);
	return nombre_archivo;
}

char* recv_manejo_f_close(int fd_modulo){
	return recv_manejo_f_open(fd_modulo);
}

t_list* recv_manejo_f_truncate(int fd_modulo){
	return recibir_paquete(fd_modulo);
}

t_list* recv_manejo_f_seek(int fd_modulo){
	return recibir_paquete(fd_modulo);
}

t_list* recv_manejo_f_read(int fd_modulo){
	return recibir_paquete(fd_modulo);
}

t_list* recv_manejo_f_write(int fd_modulo){
	return recibir_paquete(fd_modulo);
}

void send_confirmacion_archivo_creado(int fd_modulo){
	t_paquete* paquete = crear_paquete(ARCHIVO_CREADO);
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

void send_confirmacion_archivo_abierto(int fd_modulo){
	t_paquete* paquete = crear_paquete(ARCHIVO_ABIERTO);
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

void send_aviso_archivo_inexistente(int fd_modulo){
	t_paquete* paquete = crear_paquete(ARCHIVO_INEXISTENTE);
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

void send_fin_operacion_no_bloqueante(int fd_modulo){
	t_paquete* paquete = crear_paquete(FIN_OPERACION_NO_BLOQUEANTE);
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

void send_fin_operacion_bloqueante(int fd_modulo){
	t_paquete* paquete = crear_paquete(FIN_OPERACION_BLOQUEANTE);
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}


void send_iniciar_compactacion(int fd_modulo){
	t_paquete* paquete = crear_paquete(INICIAR_COMPACTACION);
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

void recv_iniciar_compactacion(int fd_modulo){
	op_code cop = recibir_operacion(fd_modulo);
	if(cop != INICIAR_COMPACTACION){
		return;
	}
	t_list* paquete = recibir_paquete(fd_modulo);
	list_destroy(paquete);
}

void send_ts_wrappers(t_list* ts_wrappers, int fd_modulo){
	t_paquete* paquete = crear_paquete(TS_WRAPPERS);
	empaquetar_ts_wrappers(paquete, ts_wrappers);
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

t_list* recv_ts_wrappers(int fd_modulo){
	op_code cop = recibir_operacion(fd_modulo);
	if(cop != TS_WRAPPERS){
		return NULL;
	}
	t_list* paquete = recibir_paquete(fd_modulo);
	t_list* ts_wrappers = desempaquetar_ts_wrappers(paquete, 0);
	list_destroy(paquete);
	return ts_wrappers;
}

void empaquetar_ts_wrappers(t_paquete* paquete, t_list* ts_wrappers){
	int cantidad_wrappers = list_size(ts_wrappers);
	agregar_a_paquete(paquete, &cantidad_wrappers, sizeof(int));
	for(int i = 0; i < cantidad_wrappers; i++){
		ts_wrapper* wrapper = list_get(ts_wrappers, i);
		agregar_a_paquete(paquete, &(wrapper->pid), sizeof(int));
		empaquetar_tabla_segmentos(paquete, wrapper->tabla_de_segmentos);
	}
}

t_list* desempaquetar_ts_wrappers(t_list* paquete, int comienzo){
	t_list* lista_ts_wrappers = list_create();
	int* cantidad_wrappers = list_get(paquete, comienzo);
	int i = comienzo + 1;
	while(i - comienzo - 1 < *(cantidad_wrappers) * 2){
		ts_wrapper* wrapper = malloc(sizeof(ts_wrapper));

		int* pid = list_get(paquete, i);
		wrapper->pid = *pid;
		i++;

		t_list* tabla_segmentos = desempaquetar_tabla_segmentos(paquete, i);
		wrapper->tabla_de_segmentos = tabla_segmentos;
		i++; // int cant_segmentos empaquetado
		i+=list_size(tabla_segmentos)*3; // i+=3 por cada segmento

		list_add(lista_ts_wrappers, wrapper);
	}
	free(cantidad_wrappers);
	return lista_ts_wrappers;
}

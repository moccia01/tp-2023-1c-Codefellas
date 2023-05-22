#include "../include/protocolo.h"

void* serializar_paquete(t_paquete* paquete, int bytes)
{
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

void enviar_mensaje(char* mensaje, int socket_cliente)
{
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

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void* recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(t_log* logger, int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje: %s", buffer);
	free(buffer);
}

t_list* recibir_paquete(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
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

void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

t_paquete* crear_paquete(op_code codigo_op)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = codigo_op;
	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
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
		i++;

		char* param1 = list_get(paquete, i);
		instruccion->parametro1 = malloc(strlen(param1));
		strcpy(instruccion->parametro1, param1);
		i++;

		char* param2 = list_get(paquete, i);
		instruccion->parametro2 = malloc(strlen(param2));
		strcpy(instruccion->parametro2, param2);
		i++;

		char* param3 = list_get(paquete, i);
		instruccion->parametro3 = malloc(strlen(param3));
		strcpy(instruccion->parametro3, param3);
		i++;

		list_add(instrucciones, instruccion);
	}
	return instrucciones;
}

void send_instrucciones(int fd_modulo,t_list* lista_de_instrucciones){
	t_paquete* instrucciones_a_mandar = crear_paquete(INSTRUCCIONES_CONSOLA);
	empaquetar_instrucciones(instrucciones_a_mandar, lista_de_instrucciones);
	enviar_paquete(instrucciones_a_mandar, fd_modulo);
}

t_list* recv_instrucciones(t_log* logger, int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	t_list* instrucciones = desempaquetar_instrucciones(paquete, 0);
	log_info(logger, "Se recibió una lista de instrucciones.");
	return instrucciones;
}

void empaquetar_tabla_segmentos(t_paquete* paquete, t_list* tabla_segmentos){
	int cantidad_segmentos = list_size(tabla_segmentos);
	agregar_a_paquete(paquete, &(cantidad_segmentos), sizeof(int));
	for(int i = 0; i < cantidad_segmentos; i++){
		t_segmento* segmento = list_get(tabla_segmentos, i);
		agregar_a_paquete(paquete, segmento, sizeof(t_segmento));
	}
}

t_list* desempaquetar_tabla_segmentos(t_list* paquete, int comienzo){
	t_list* tabla_segmentos = list_create();
	int* cantidad_segmentos = list_get(paquete, comienzo);
	for(int i = comienzo + 1; i < *cantidad_segmentos; i++){
		t_segmento* segmento = list_get(paquete, i);
		list_add(tabla_segmentos, segmento);
	}
	return tabla_segmentos;
}

void empaquetar_contexto_ejecucion(t_paquete* paquete, t_contexto_ejecucion* contexto){
	agregar_a_paquete(paquete, &(contexto->pid), sizeof(int));
	agregar_a_paquete(paquete, &(contexto->program_counter), sizeof(int));
	agregar_a_paquete(paquete, &(contexto->estado), sizeof(estado_proceso));
	agregar_a_paquete(paquete, &(contexto->motivo_exit), sizeof(motivo_exit));
	agregar_a_paquete(paquete, &(contexto->motivo_block), sizeof(motivo_block));
	empaquetar_instrucciones(paquete, contexto->instrucciones);
	empaquetar_tabla_segmentos(paquete, contexto->tabla_de_segmentos);
}

t_contexto_ejecucion* desempaquetar_contexto_ejecucion(t_list* paquete){
	t_contexto_ejecucion* contexto = malloc(sizeof(t_contexto_ejecucion));
	int* pid = list_get(paquete, 0);
	contexto->pid = *pid;

	int* program_counter = list_get(paquete, 1);
	contexto->program_counter = *program_counter;

	estado_proceso* estado =  list_get(paquete, 2);
	contexto->estado = *estado;

	motivo_exit* motivo_exit = list_get(paquete, 3);
	contexto->motivo_exit = *motivo_exit;

	motivo_block* motivo_block = list_get(paquete, 4);
	contexto->motivo_block = *motivo_block;

	t_list* instrucciones = desempaquetar_instrucciones(paquete, 5);
	contexto->instrucciones = instrucciones;
	int cantidad_instrucciones = list_size(instrucciones);

	t_list* tabla_segmentos = desempaquetar_tabla_segmentos(paquete, 5 + (cantidad_instrucciones * 4) + 1);
	contexto->tabla_de_segmentos = tabla_segmentos;

	return contexto;
}

void send_contexto_ejecucion(t_contexto_ejecucion* contexto, int fd_modulo){
	t_paquete* paquete = crear_paquete(CONTEXTO_EJECUCION);
	empaquetar_contexto_ejecucion(paquete, contexto);
	enviar_paquete(paquete, fd_modulo);
}

t_contexto_ejecucion* recv_contexto_ejecucion(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	t_contexto_ejecucion* contexto_recibido = desempaquetar_contexto_ejecucion(paquete);
	return contexto_recibido;
}

void send_cambiar_estado(t_contexto_ejecucion* contexto, int fd_modulo){
	t_paquete* paquete = crear_paquete(CAMBIAR_ESTADO);
	empaquetar_contexto_ejecucion(paquete, contexto);
	enviar_paquete(paquete, fd_modulo);
}

t_contexto_ejecucion* recv_cambiar_estado(int fd_modulo){
	t_contexto_ejecucion* contexto_recibido = recv_contexto_ejecucion(fd_modulo);
	return contexto_recibido;
}

void empaquetar_tiempo_io(t_paquete* paquete, char* tiempo_io){
	int tiempo = atoi(tiempo_io);
	agregar_a_paquete(paquete, &(tiempo), sizeof(int));
}

void send_tiempo_io(char* tiempo_io, int fd_modulo){
	t_paquete* paquete = crear_paquete(MANEJAR_IO);
	empaquetar_tiempo_io(paquete, tiempo_io);
	enviar_paquete(paquete, fd_modulo);
}

void empaquetar_recurso(t_paquete* paquete, char* recurso){
	agregar_a_paquete(paquete, recurso, strlen(recurso) + 1);
}

void send_recurso_wait(char* recurso, int fd_modulo){
	t_paquete* paquete = crear_paquete(MANEJAR_WAIT);
	empaquetar_recurso(paquete, recurso);
	enviar_paquete(paquete, fd_modulo);
}

void send_recurso_signal(char* recurso, int fd_modulo){
	t_paquete* paquete = crear_paquete(MANEJAR_SIGNAL);
	empaquetar_recurso(paquete, recurso);
	enviar_paquete(paquete, fd_modulo);
}

int recv_tiempo_io(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	int* tiempo = list_get(paquete, 0);
	return *tiempo;
}

char* recv_recurso(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	char* recurso = list_get(paquete, 0);
	return recurso;
}


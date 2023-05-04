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

void send_instrucciones(int fd_modulo,t_list* lista_de_instrucciones){
	t_paquete* instrucciones_a_mandar = crear_paquete(INSTRUCCIONES_CONSOLA);
	for(int i=0; i<list_size(lista_de_instrucciones); i++){
		t_instruccion* instruccion = list_get(lista_de_instrucciones, i);
		agregar_a_paquete(instrucciones_a_mandar, instruccion, sizeof(instruccion));
	}
	enviar_paquete(instrucciones_a_mandar, fd_modulo);
}

t_list* recv_instrucciones(t_log* logger, int fd_modulo){
	log_info(logger, "Se recibió una lista de instrucciones.");
	return recibir_paquete(fd_modulo);
}

void send_contexto_ejecucion(t_contexto_ejecucion* contexto, int fd_modulo){
	t_paquete* contexto_a_mandar = crear_paquete(CONTEXTO_EJECUCION);
	agregar_a_paquete(contexto_a_mandar, contexto, sizeof(contexto));
	enviar_paquete(contexto_a_mandar, fd_modulo);
	eliminar_paquete(contexto_a_mandar);
}

t_contexto_ejecucion* recv_contexto_ejecucion(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	return list_get(paquete, 0);
}

void send_cambiar_estado(t_contexto_ejecucion* contexto, int fd_modulo){

	t_paquete* contexto_a_mandar = crear_paquete(CAMBIAR_ESTADO);
	agregar_a_paquete(contexto_a_mandar, contexto, sizeof(t_contexto_ejecucion));
	enviar_paquete(contexto_a_mandar, fd_modulo);

}

t_contexto_ejecucion* recv_cambiar_estado(int fd_modulo){
	return recv_contexto_ejecucion(fd_modulo);
}

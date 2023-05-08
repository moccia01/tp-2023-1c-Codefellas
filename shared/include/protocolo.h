#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>
#include"shared.h"

typedef enum
{
	MENSAJE,
	INSTRUCCIONES_CONSOLA,
	PAQUETE,
	CONTEXTO_EJECUCION,
	CAMBIAR_ESTADO,
}op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

//Mensajes
void enviar_mensaje(char* mensaje, int socket_cliente);
int recibir_operacion(int socket_cliente);
void* recibir_buffer(int* size, int socket_cliente);
void recibir_mensaje(t_log* logger, int socket_cliente);
void crear_buffer(t_paquete* paquete);

//Paquetes
t_list* recibir_paquete(int);
t_paquete* crear_paquete(op_code);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void agregar_a_paquete_con_header(t_paquete *paquete, void *valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
void* serializar_paquete(t_paquete* paquete, int bytes);

// LA EMPAQUETASION O.o
void empaquetar_instrucciones(t_paquete* instrucciones_a_mandar, t_list* lista_de_instrucciones);
t_list* desempaquetar_instrucciones(t_list* paquete, int comienzo);
void empaquetar_tabla_segmentos(t_paquete* paquete, t_list* tabla_segmentos);
t_list* desempaquetar_tabla_segmentos(t_list* paquete, int comienzo);
void empaquetar_contexto_ejecucion(t_paquete* paquete, t_contexto_ejecucion* contexto);
t_contexto_ejecucion* desempaquetar_contexto_ejecucion(t_list* paquete);

// Sends
void send_instrucciones(int fd_modulo, t_list* lista_de_instrucciones);
void send_contexto_ejecucion(t_contexto_ejecucion* contexto, int fd_modulo);
void send_cambiar_estado(t_contexto_ejecucion* contexto, int fd_modulo);

// Recvs
t_list* recv_instrucciones(t_log* logger, int fd_modulo);
t_contexto_ejecucion* recv_contexto_ejecucion(int fd_modulo);
t_contexto_ejecucion* recv_cambiar_estado(int fd_modulo);

#endif /* PROTOCOLO_H_ */


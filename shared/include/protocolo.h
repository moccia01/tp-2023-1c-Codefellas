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

typedef enum{
	MENSAJE,
	INSTRUCCIONES_CONSOLA,
	PAQUETE,
	CONTEXTO_EJECUCION,
	CAMBIAR_ESTADO,
	MANEJAR_IO,
	MANEJAR_WAIT,
	MANEJAR_SIGNAL,
	MANEJAR_F_OPEN,
	MANEJAR_F_CREATE,
	MANEJAR_F_CLOSE,
	MANEJAR_F_SEEK,
	MANEJAR_F_READ,
	FIN_F_READ,
	MANEJAR_F_WRITE,
	FIN_F_WRITE,
	MANEJAR_F_TRUNCATE,
	MANEJAR_CREATE_SEGMENT,
	MANEJAR_DELETE_SEGMENT,
	TABLA_SEGMENTOS,
	INICIALIZAR_PROCESO,
	FINALIZAR_PROCESO,
	BASE_SEGMENTO,
	SEGMENT_RESPONSE,
	PEDIDO_LECTURA,
	PEDIDO_ESCRITURA,
	ARCHIVO_ABIERTO,
	ARCHIVO_INEXISTENTE,
	ARCHIVO_CREADO,
	INICIAR_COMPACTACION,
	TS_WRAPPERS,
}op_code;

typedef struct{
	int size;
	void* stream;
} t_buffer;

typedef struct{
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
void empaquetar_registro_contexto(t_paquete* paquete, t_registros* registros);
t_registros* desempaquetar_registros(t_list* paquete, int comienzo);
void empaquetar_segmento(t_paquete* paquete, t_segmento* segmento);
t_segmento* desempaquetar_segmento(t_list* paquete, int comienzo);
void empaquetar_ts_wrappers(t_paquete* paquete, t_list* ts_wrappers);
t_list* desempaquetar_ts_wrappers(t_list* paquete, int comienzo);

//Sends
void send_instrucciones(int fd_modulo, t_list* lista_de_instrucciones);
void send_contexto_ejecucion(t_contexto_ejecucion* contexto, int fd_modulo);
void send_cambiar_estado(estado_proceso estado, int fd_modulo);
void send_tiempo_io(int tiempo_io, int fd_modulo);
void send_recurso_wait(char* recurso, int fd_modulo);
void send_recurso_signal(char* recurso, int fd_modulo);
void send_manejar_f_open(char* nombre_archivo, int fd_modulo);
void send_manejar_f_close(char* nombre_archivo, int fd_modulo);
void send_manejar_f_seek(char* nombre_archivo, int fd_modulo);
void send_manejar_f_read(char* nombre_archivo, int dir_fisica, int cantidad_bytes, int fd_modulo);
void send_manejar_f_write(char* nombre_archivo, int dir_fisica, int cantidad_bytes, int fd_modulo);
void send_manejar_f_wait(char* nombre_archivo, int fd_modulo);
void send_manejar_f_truncate(char* nombre_archivo, int tamanio, int fd_modulo);
void send_create_segment(int pid, int id_segmento, int tamanio, int fd_modulo);
void send_segment_response(t_segment_response resp, int fd_modulo);
void send_delete_segment(int pid, int id_segmento, int fd_modulo);
void send_tabla_segmentos(t_list* tabla_segmentos, int fd_modulo);
void send_leer_valor(int dir_fisica, int tamaio_a_leer,int fd_modulo);
void send_escribir_valor(char* valor, int dir_fisica, int fd_modulo);
void send_inicializar_proceso(int pid, int fd_modulo);
void send_proceso_inicializado(t_list* tabla_segmentos, int fd_modulo);
void send_terminar_proceso(int pid, int fd_modulo);
void send_base_segmento(int base, int fd_modulo);
//void send_confirmacion(int fd_modulo);
void send_confirmacion_archivo_creado(int fd_modulo);
void send_confirmacion_archivo_abierto(int fd_modulo);
void send_aviso_archivo_inexistente(int fd_modulo);
void send_iniciar_compactacion(int fd_modulo);
void send_ts_wrappers(t_list* ts_wrappers, int fd_modulo);
void send_valor_leido(char* valor, int fd_modulo);

//Recvs
t_list* recv_instrucciones(t_log* logger, int fd_modulo);
t_contexto_ejecucion* recv_contexto_ejecucion(int fd_modulo);
estado_proceso recv_cambiar_estado(int fd_modulo);
int recv_tiempo_io(int fd_modulo);
char* recv_recurso(int fd_modulo);
char* recv_valor(int fd_modulo);
t_list* recv_leer_valor(int fd_modulo);
t_list* recv_escribir_valor(int fd_modulo);
t_list* recv_create_segment(int fd_modulo);
t_segment_response recv_segment_response(int fd_modulo);
t_list* recv_delete_segment(int fd_modulo);
t_list* recv_tabla_segmentos(int fd_modulo);
int recv_inicializar_proceso(int fd_modulo);
t_list* recv_proceso_inicializado(int fd_modulo);
int recv_terminar_proceso(int fd_modulo);
int recv_base_segmento(int fd_modulo);
char* recv_nombre_archivo(int fd_modulo);
int recv_tamanio(int fd_modulo);
void recv_iniciar_compactacion(int fd_modulo);
t_list* recv_ts_wrappers(int fd_modulo);

// Destroyers
void contexto_destroyer(t_contexto_ejecucion* contexto);
void registros_destroy(t_registros* registros);
void instruccion_destroyer(t_instruccion* instruccion);
void segmento_destroy(t_segmento* segmento);
void lista_instrucciones_destroy(t_list* lista);

#endif /* PROTOCOLO_H_ */


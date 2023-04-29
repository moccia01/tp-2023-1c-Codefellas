#ifndef CPU_H_
#define CPU_H_

#include <commons/log.h>
#include <commons/config.h>
#include <sockets.h>
#include <protocolo.h>
#include <shared.h>

#endif /* CPU_H_ */

t_registros* inicializar_registro();
void fetch();
void decode();

//Instrucciones	VERIFICAR TIPOS DE RETORNO
void set(t_registros registro, char valor);		//Verificar tipo de valor
void mov_in(t_registros registro, int dir_logica);
void mov_out(int dir_logica, t_registros registro);
void i_o(int tiempo);
void f_open(char nombre_archivo);			//Verificar tipo de nombre de archivo
void f_close(char nombre_archivo);
void f_seek(char nombre_archivo);
void f_read(char nombre_archivo);
void f_write(char nombre_archivo);
void f_truncate(char nombre_De_Archivo, int tamanio);
void wait();		//Verificar el tipo de recurso
void signal();		//Verificar el tipo de recurso
void create_segment(int id_segmento, int tamanio);
void delete_segment(int id_segmento);
void yield();
void exit();

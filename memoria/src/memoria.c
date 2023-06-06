#include "../include/memoria.h"


int main(int argc, char **argv) {
	if (argc > 2) {
		return EXIT_FAILURE;
	}
	logger = log_create("memoria.log", "memoria_main", true, LOG_LEVEL_INFO);
	config = config_create(argv[1]);
	if(config == NULL){
		log_error(logger, "No se encontró el archivo :(");
		exit(1);
	}
	leer_config();
	inicializar_memoria();
	// Inicio servidor
	fd_memoria = iniciar_servidor(logger, IP, PUERTO);

	while(server_escuchar(fd_memoria));

	// Termino programa
	terminar_programa(logger, config);

	return 0;
}

// --------------------- INIT ---------------------

void leer_config(){
	IP = config_get_string_value(config, "IP_ESCUCHA");
	PUERTO = config_get_string_value(config, "PUERTO_ESCUCHA");
	TAM_MEMORIA = config_get_int_value(config, "TAM_MEMORIA");
	TAM_SEGMENTO_0 = config_get_int_value(config, "TAM_SEGMENTO_0");
	CANT_SEGMENTOS = config_get_int_value(config, "CANT_SEGMENTOS");
	RETARDO_MEMORIA = config_get_int_value(config, "RETARDO_MEMORIA");
	RETARDO_COMPACTACION = config_get_int_value(config, "RETARDO_COMPACTACION");
	char *algoritmo_memoria = config_get_string_value(config, "ALGORITMO_ASIGNACION");
	asignar_algoritmo_memoria(algoritmo_memoria);
}

void asignar_algoritmo_memoria(char *algoritmo_memoria) {
	if (strcmp(algoritmo_memoria, "FIRST") == 0) {
		ALGORITMO_ASIGNACION = FIRST_FIT;
	} else if (strcmp(algoritmo_memoria, "BEST") == 0) {
		ALGORITMO_ASIGNACION = BEST_FIT;
	}else if((strcmp(algoritmo_memoria, "WORST") == 0)){
		ALGORITMO_ASIGNACION = WORST_FIT;
	}
	else{
		log_error(logger, "El algoritmo no es valido");
	}
}

void terminar_programa(){
	log_destroy(logger);
	config_destroy(config);
}


// --------------------- COMUNICACION ---------------------

static void procesar_conexion(void *void_args) {
	int *args = (int*) void_args;
	int cliente_socket = *args;

	op_code cop;
	while (cliente_socket != -1) {
		if (recv(cliente_socket, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
			log_info(logger, "El cliente se desconecto de %s server", server_name);
			return;
		}
		switch (cop) {
		case MENSAJE:
			recibir_mensaje(logger, cliente_socket);
			break;
		case PAQUETE:
			t_list *paquete_recibido = recibir_paquete(cliente_socket);
			log_info(logger, "Recibí un paquete con los siguientes valores: ");
			list_iterate(paquete_recibido, (void*) iterator);
			break;
		case MANEJAR_CREATE_SEGMENT:
			t_list* create_sgm_params = recv_create_segment(cliente_socket);
//			int* id = list_get(create_sgm_params, 0);
			int* tamanio = list_get(create_sgm_params, 1);
			t_segment_response verificacion_espacio = verificar_espacio_memoria(*tamanio);
			switch(verificacion_espacio){
			case SEGMENT_CREATED: //crear segmento
				break;
			case OUT_OF_MEM: //avisar a kernel que out_of_mem
				break;
			case COMPACT: //solicitar compactacion y cuando kernel de el ok, compactar.
				break;
			}
			break;
		case MANEJAR_DELETE_SEGMENT:
			// ...
			break;
		case INICIALIZAR_PROCESO:
			int pid_init = recv_inicializar_proceso(cliente_socket);
			log_info(logger, "se inicializa proceso");
			t_list* tabla_segmentos_inicial = inicializar_proceso(pid_init);
			send_proceso_inicializado(tabla_segmentos_inicial, cliente_socket);
			break;
		case FINALIZAR_PROCESO:
			int pid_fin = recv_terminar_proceso(cliente_socket);
			// hacer lo q haya q hacer segun el enunciado para finalizar un proceso.
			terminar_proceso(pid_fin);
			break;
		case PEDIDO_LECTURA_CPU:
//			t_list* parametros_lectura_cpu= recv_pedido_lectura(cliente_socket);
//			void* valor_leido_cpu = espacio_usuario[posicion_cpu];
//			send_valor_leido(valor_leido_cpu, cliente_socket);
			break;
		case PEDIDO_ESCRITURA_CPU:
//			t_list* parametros_escritura_cpu = recv_pedido_escritura(cliente_socket);
//			int* posicion_escritura_cpu = list_get(parametros_escritura, 0);
//			void* valor_a_escribir_cpu = list_get(parametros_escritura, 1);
//			espacio_usuario[*posicion_escritura_cpu] = valor_a_escribir_cpu;
			break;
		case PEDIDO_LECTURA_FS:
//			t_list* parametros_lectura_fs = recv_pedido_lectura(cliente_socket);
//			void* valor_leido_fs = espacio_usuario[posicion_fs];
//			send_valor_leido(valor_leido_fs, cliente_socket);
			break;
		case PEDIDO_ESCRITURA_FS:
//			t_list* parametros_escritura_fs = recv_pedido_escritura(cliente_socket);
//			int* posicion_escritura_fs = list_get(parametros_escritura, 0);
//			void* valor_a_escribir_fs = list_get(parametros_escritura, 1);
//			espacio_usuario[*posicion_escritura_fs] = valor_a_escribir_fs;
			break;
		default:
				log_error(logger, "Codigo de operacion no reconocido en el server de %s", server_name);
				return;
			}

		}
	log_warning(logger, "El cliente se desconecto de %s server", server_name);
	return;
}

void iterator(char *value) {
	log_info(logger, "%s", value);
}

int server_escuchar(int server_socket) {
	server_name = "Memoria";
	int cliente_socket = esperar_cliente(logger, server_name, server_socket);

	if (cliente_socket != -1) {
		pthread_t hilo;
		int *args = malloc(sizeof(int));
		args = &cliente_socket;
		pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) args);
		pthread_detach(hilo);
		return 1;
	}
	return 0;
}

t_segment_response verificar_espacio_memoria(int tamanio){

	return SEGMENT_CREATED;
}

/*
int algoritmo_first(int tamano){
	int base=-1;
	while(lista){
		if(tamano < (lista->tamanio))
			return base;
	}
	return base;
}
*/
void inicializar_memoria(){
	log_info(logger, "Creando espacio de memoria...");
//	espacio_usuario[TAM_MEMORIA];

	lista_ts_wrappers = list_create();

	segmento_0 = malloc(sizeof(t_segmento));
	segmento_0->base = 0;
	segmento_0->id = 0;
	segmento_0->tamanio = TAM_SEGMENTO_0;

}

t_list* inicializar_proceso(int pid){
	t_list* tabla_segmentos_inicial = list_create();
	list_add(tabla_segmentos_inicial, segmento_0);

	// añadir el pid y su tabla a la lista global de pids y tablas
	ts_wrapper* wrapper = malloc(sizeof(ts_wrapper));
	wrapper->pid = pid;
	wrapper->tabla_de_segmentos = tabla_segmentos_inicial;
	list_add(lista_ts_wrappers, wrapper);

	// añadir el pid y su lista de escrituras
	t_escrituras* escrituras = malloc(sizeof(t_escrituras));
	escrituras->pid = pid;
	escrituras->escrituras = list_create();
	return tabla_segmentos_inicial;
}

// creamos lista por proceso que a su vez tiene segmentos
// cada proceso agregarle segmento_0

void terminar_proceso(int pid){
	eliminar_escrituras_de_proceso(pid);
	eliminar_tabla_segmentos(pid);
}

void eliminar_escrituras_de_proceso(int pid){

}

void eliminar_tabla_segmentos(int pid){

}

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
			int* id = list_get(create_sgm_params, 0);
			int* tamanio = list_get(create_sgm_params, 1);
			int* pid_cs = list_get(create_sgm_params, 2);
			log_info(logger, "manejo create_segment");
			t_segment_response verificacion_espacio = verificar_espacio_memoria(*tamanio);
			switch(verificacion_espacio){
			case SEGMENT_CREATED: //crear segmento
				log_info(logger, "hay espacio contiguo para crear el segmento");
				send_segment_response(verificacion_espacio, cliente_socket);
				int direccion = crear_segmento_segun_algoritmo(*id, *tamanio, *pid_cs);
				log_info(logger, "se creo un segmento en la direccion %d", direccion);
				send_base_segmento(direccion, cliente_socket);
				break;
			case OUT_OF_MEM:
				log_info(logger, "no hay espacio contiguo para crear el segmento");
				send_segment_response(verificacion_espacio, cliente_socket);
				break;
			case COMPACT:
				log_info(logger, "hay q compactar");
				send_segment_response(verificacion_espacio, cliente_socket);
				// esperar a que kernel de el ok para compactar y recien ahi compactar
				recv_iniciar_compactacion(cliente_socket);
//				compactar();
//				send_ts_wrappers(lista_ts_wrappers, cliente_socket);
				break;
			}
			break;
		case MANEJAR_DELETE_SEGMENT:
			t_list* delete_sgm_params = recv_delete_segment(cliente_socket);
			int* id_segmento = list_get(delete_sgm_params, 0);
			int* pid_ds = list_get(delete_sgm_params, 1);
			log_info(logger, "manejo delete_segment");
			t_list* tabla_segmentos_actualizada = deletear_segmento(*id_segmento, *pid_ds);
			log_info(logger, "mando tabla actualizada de tamaño %d", list_size(tabla_segmentos_actualizada));
			send_tabla_segmentos(tabla_segmentos_actualizada, cliente_socket);
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
	int tamanio_total=0;
	for(int i = 0; i < list_size(huecos_libres); i++){
		t_hueco_memoria* hueco_libre = list_get(huecos_libres, i);
		if(hueco_libre->tamanio >= tamanio){
			return SEGMENT_CREATED;
		}
		tamanio_total+=hueco_libre->tamanio;
	}
	if(tamanio_total>=tamanio)
		return COMPACT;
	return OUT_OF_MEM;
}

void inicializar_memoria(){
	log_info(logger, "Creando espacio de memoria...");
	espacio_usuario = malloc(TAM_MEMORIA);

	lista_ts_wrappers = list_create();
	huecos_libres = list_create();

	segmento_0 = malloc(sizeof(t_segmento));
	segmento_0->base = 0;
	segmento_0->id = 0;
	segmento_0->tamanio = TAM_SEGMENTO_0;

	t_hueco_memoria* hueco_libre_inicial = malloc(sizeof(t_hueco_memoria));
	hueco_libre_inicial->base = TAM_SEGMENTO_0;
	hueco_libre_inicial->tamanio = TAM_MEMORIA - TAM_SEGMENTO_0;
	list_add(huecos_libres, hueco_libre_inicial);

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
//	t_escrituras* escrituras = malloc(sizeof(t_escrituras));
//	escrituras->pid = pid;
//	escrituras->escrituras = list_create();

	return tabla_segmentos_inicial;
}

// creamos lista por proceso que a su vez tiene segmentos
// cada proceso agregarle segmento_0

void terminar_proceso(int pid){
	eliminar_tabla_segmentos(pid);
}

void eliminar_tabla_segmentos(int pid){
	//TO-DO: ya se puede implementar
	for(int i = 0; i < list_size(lista_ts_wrappers); i++){
		ts_wrapper *tabla_segmento = list_get(lista_ts_wrappers, i);
		if(tabla_segmento->pid==pid){
			list_remove_element(lista_ts_wrappers, tabla_segmento);
		}
	}
	return;
}

int crear_segmento_segun_algoritmo(int id, int tamanio, int pid){
	t_hueco_memoria* hueco;
	switch(ALGORITMO_ASIGNACION){
	case FIRST_FIT:
		hueco = encontrar_hueco_first(tamanio);
		break;
	case WORST_FIT:
		hueco = encontrar_hueco_worst(tamanio);
		break;
	case BEST_FIT:
		hueco = encontrar_hueco_best(tamanio);
		break;
	default: break;
	}
	t_segmento* nuevo_segmento = crear_segmento(pid, id, tamanio, hueco->base);
	actualizar_hueco_libre(nuevo_segmento, hueco);
	return nuevo_segmento->base;
}
//TESTEAR FUNCION
t_hueco_memoria* encontrar_hueco_first(int tamanio){
	for(int i = 0; i < list_size(huecos_libres); i++){
		t_hueco_memoria* hueco_libre = list_get(huecos_libres, i);
		if(hueco_libre->tamanio >= tamanio){
			return hueco_libre;
		}
	}
	return NULL;
}
//TESTEAR FUNCION
t_hueco_memoria* encontrar_hueco_best(int tamanio){
	//TO-DO: ya se puede implementar
	t_hueco_memoria *aux=NULL;
	for(int i = 0; i < list_size(huecos_libres); i++){
			t_hueco_memoria* hueco_libre = list_get(huecos_libres, i);
			if(!aux){
				if(hueco_libre->tamanio >= tamanio){
					aux=hueco_libre;
				}
			}
			else{
				if(hueco_libre->tamanio >= tamanio && hueco_libre->tamanio<aux->tamanio){
					aux=hueco_libre;
				}
			}
		}
	return aux;
}
//TESTEAR FUNCION
t_hueco_memoria* encontrar_hueco_worst(int tamanio){
	//TO-DO: ya se puede implementar
	t_hueco_memoria *aux=NULL;
		for(int i = 0; i < list_size(huecos_libres); i++){
				t_hueco_memoria* hueco_libre = list_get(huecos_libres, i);
				if(!aux){
					if(hueco_libre->tamanio >= tamanio){
						aux=hueco_libre;
					}
				}
				else{
					if(hueco_libre->tamanio >= tamanio && hueco_libre->tamanio>aux->tamanio){
						aux=hueco_libre;
					}
				}
			}
		return aux;
}

t_segmento* crear_segmento(int pid, int id, int base, int tamanio){
	t_segmento* segmento = malloc(sizeof(t_segmento));
	segmento->id = id;
	segmento->base = base;
	segmento->tamanio = tamanio;

	actualizar_tabla_segmentos_de_proceso(pid, segmento);
//	list_add(segmentos_en_memoria, segmento);

	return segmento;
}

void actualizar_hueco_libre(t_segmento* segmento_nuevo, t_hueco_memoria* hueco_viejo){
	hueco_viejo->base = segmento_nuevo->base + segmento_nuevo->tamanio;
	hueco_viejo->tamanio = hueco_viejo->tamanio - segmento_nuevo->tamanio;
	if(hueco_viejo->tamanio == 0){
		list_remove_element(huecos_libres, hueco_viejo);
	}
}

void actualizar_tabla_segmentos_de_proceso(int pid, t_segmento* segmento){
	//TO-DO: ya se puede implementar
	for(int i = 0; i < list_size(lista_ts_wrappers); i++){
		ts_wrapper *ts_proceso = list_get(lista_ts_wrappers, i);
		if(ts_proceso->pid==pid){
			list_add(ts_proceso->tabla_de_segmentos,segmento);
			//enviar_tabla_a_kernel(pid,ts_proceso->tabla_de_segmentos);
		}
	}
	return;
}
/*
void enviar_tabla_a_kernel(int pid, t_list *tabla_de_segmentos){
//TODO: Que hacemo'?
}
*/

t_list* deletear_segmento(int id_segmento, int pid){
	int base;
	int tamanio;
	t_list* ts_actualizada;
	for(int i = 0; i < list_size(lista_ts_wrappers); i++){
		ts_wrapper *ts_proceso = list_get(lista_ts_wrappers, i);
		if(ts_proceso->pid==pid){
			ts_actualizada = ts_proceso->tabla_de_segmentos;
			for(int j = 0; j < list_size(ts_proceso->tabla_de_segmentos); j++){
				t_segmento *segmento = list_get(ts_proceso->tabla_de_segmentos, j);
				if(segmento->id == id_segmento){
					base = segmento->base;
					tamanio = segmento->tamanio;
					list_remove(ts_proceso->tabla_de_segmentos, j);
				}
			}
		}
	}
	agregar_hueco_libre(base, tamanio);
	return ts_actualizada;
	//estas deleteado papaaa
}

void agregar_hueco_libre(int base, int tamanio){
	t_segmento *aux = NULL;
	int i;
	t_segmento* hueco_nuevo = malloc(sizeof(t_segmento));
	hueco_nuevo -> base = base;
	hueco_nuevo -> tamanio = tamanio;

	for(i=0 ; i < list_size(huecos_libres); i++){
		t_segmento *segmento = list_get(huecos_libres, i);
		if(segmento->base < base){
			aux = segmento;
		}
		else
			break;
	}
	if(aux == NULL){
		list_add_in_index(huecos_libres, 0, hueco_nuevo);
		aux = hueco_nuevo;
	}else{
		if(aux->base + aux->tamanio == base){
			aux->tamanio += tamanio;
			free(hueco_nuevo);
		}
		else{
			list_add_in_index(huecos_libres,i,hueco_nuevo);
			aux = hueco_nuevo;
		}
	}
	if(i + 1 < list_size(huecos_libres)){
		t_segmento *siguiente_hueco = list_get(huecos_libres, i+1);
		if(aux->base + aux->tamanio == siguiente_hueco->base){
			aux->tamanio += siguiente_hueco->tamanio;
			list_remove(huecos_libres, i+1);
		}
	}
}



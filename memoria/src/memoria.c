#include "../include/memoria.h"


int main(int argc, char **argv) {
	if (argc > 2) {
		return EXIT_FAILURE;
	}
	config = config_create(argv[1]);
	logger = log_create("memoria.log", "memoria_main", true, LOG_LEVEL_INFO);
	if(config == NULL){
		log_error(logger, "No se encontró el archivo :(");
		config_destroy(config);
		log_destroy(logger);
		exit(1);
	}
	logger_obligatorio = log_create("memoria.log", "memoria_obligatorio", true, LOG_LEVEL_INFO);
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
				log_info(logger_obligatorio, "PID: %d - Crear Segmento: %d - Base: %d - TAMAÑO: %d", *pid_cs, *id, direccion, *tamanio);
				send_base_segmento(direccion, cliente_socket);
				break;
			case OUT_OF_MEM:
				log_info(logger, "no hay espacio contiguo para crear el segmento");
				send_segment_response(verificacion_espacio, cliente_socket);
				break;
			case COMPACT:
				log_info(logger, "hay q compactar");
				send_segment_response(verificacion_espacio, cliente_socket);
				recv_iniciar_compactacion(cliente_socket);
				log_info(logger_obligatorio, "Solicitud de Compactación");
				compactar();
				usleep(RETARDO_COMPACTACION * 1000);
				log_info(logger_obligatorio, "Resultado compactacion: ");
				log_resultado_compactacion();
				send_ts_wrappers(lista_ts_wrappers, cliente_socket);
				break;
			}
			break;
		case MANEJAR_DELETE_SEGMENT:
			t_list* delete_sgm_params = recv_delete_segment(cliente_socket);
			int* id_segmento = list_get(delete_sgm_params, 0);
			int* pid_ds = list_get(delete_sgm_params, 1);
			t_list* tabla_segmentos_actualizada = deletear_segmento(*id_segmento, *pid_ds);
			log_info(logger, "mando tabla actualizada de tamaño %d", list_size(tabla_segmentos_actualizada));
			send_tabla_segmentos(tabla_segmentos_actualizada, cliente_socket);
			break;
		case INICIALIZAR_PROCESO:
			int pid_init = recv_inicializar_proceso(cliente_socket);
			log_info(logger_obligatorio, "Creación de Proceso PID: %d", pid_init);
			t_list* tabla_segmentos_inicial = inicializar_proceso(pid_init);
			send_proceso_inicializado(tabla_segmentos_inicial, cliente_socket);
			break;
		case FINALIZAR_PROCESO:
			int pid_fin = recv_terminar_proceso(cliente_socket);
			log_info(logger_obligatorio, "Eliminación de Proceso PID: %d", pid_fin);
			terminar_proceso(pid_fin);
			break;
		case PEDIDO_LECTURA_CPU:
			t_list* parametros_lectura_cpu= recv_leer_valor(cliente_socket);
			int* posicion_lectura_cpu = list_get(parametros_lectura_cpu, 0);
			int* tamanio_lectura_cpu = list_get(parametros_lectura_cpu, 1);
			int* pid_lectura_cpu = list_get(parametros_lectura_cpu, 2);
			char* valor_leido_cpu = malloc(*tamanio_lectura_cpu);
			usleep(RETARDO_MEMORIA * 1000);
			memcpy(valor_leido_cpu, espacio_usuario + *posicion_lectura_cpu, *tamanio_lectura_cpu);
			log_info(logger_obligatorio, "PID: %d - Acción: LEER - Dirección física: %d - Tamaño: %d - Origen: CPU",*pid_lectura_cpu, *posicion_lectura_cpu, *tamanio_lectura_cpu);
			log_valor_espacio_usuario(valor_leido_cpu, *tamanio_lectura_cpu);
			send_valor_leido_cpu(valor_leido_cpu, *tamanio_lectura_cpu, cliente_socket);
			break;
		case PEDIDO_LECTURA_FS:
			t_list* parametros_lectura_fs = recv_leer_valor(cliente_socket);
			int* posicion_lectura_fs = list_get(parametros_lectura_fs, 0);
			int* tamanio_lectura_fs = list_get(parametros_lectura_fs, 1);
			int* pid_lectura_fs = list_get(parametros_lectura_fs, 2);
			char* valor_leido_fs = malloc(*tamanio_lectura_fs);
			usleep(RETARDO_MEMORIA * 1000);
			memcpy(valor_leido_fs, espacio_usuario + *posicion_lectura_fs, *tamanio_lectura_fs);
			log_info(logger_obligatorio, "PID: %d - Acción: LEER - Dirección física: %d - Tamaño: %d - Origen: FS", *pid_lectura_fs, *posicion_lectura_fs, *tamanio_lectura_fs);
			log_valor_espacio_usuario(valor_leido_fs, *tamanio_lectura_fs);
			send_valor_leido_fs(valor_leido_fs, *tamanio_lectura_fs, cliente_socket);
			break;
		case PEDIDO_ESCRITURA_CPU:
			t_list* parametros_escritura_cpu = recv_escribir_valor(cliente_socket);
			char* valor_a_escribir_cpu = list_get(parametros_escritura_cpu, 0);
			int* posicion_escritura_cpu = list_get(parametros_escritura_cpu, 1);
			int* tam_esc_cpu = list_get(parametros_escritura_cpu, 2);
			int* pid_escritura_cpu = list_get(parametros_escritura_cpu, 3);

			log_info(logger, "el tamaño del valor a escribir es: %d", *tam_esc_cpu);
			usleep(RETARDO_MEMORIA * 1000);
			memcpy(espacio_usuario + *posicion_escritura_cpu, valor_a_escribir_cpu, *tam_esc_cpu);
			log_info(logger_obligatorio, "PID: %d - Acción: ESCRIBIR - Dirección física: %d - Tamaño: %d - Origen: CPU", *pid_escritura_cpu, *posicion_escritura_cpu, *tam_esc_cpu);
			log_valor_espacio_usuario(valor_a_escribir_cpu, *tam_esc_cpu);
			send_fin_escritura(cliente_socket);
			break;
		case PEDIDO_ESCRITURA_FS:
			t_list* parametros_escritura_fs = recv_escribir_valor(cliente_socket);
			char* valor_a_escribir_fs = list_get(parametros_escritura_fs, 0);
			int* posicion_escritura_fs = list_get(parametros_escritura_fs, 1);
			int* tam_esc_fs = list_get(parametros_escritura_fs, 2);
			int* pid_escritura_fs = list_get(parametros_escritura_fs, 3);

			log_info(logger, "el tamaño del valor a escribir es: %d", *tam_esc_fs);
			usleep(RETARDO_MEMORIA * 1000);
			memcpy(espacio_usuario + *posicion_escritura_fs, valor_a_escribir_fs, *tam_esc_fs);
			log_info(logger_obligatorio, "PID: %d - Acción: ESCRIBIR - Dirección física: %d - Tamaño: %d - Origen: FS", *pid_escritura_fs, *posicion_escritura_fs, *tam_esc_fs);
			log_valor_espacio_usuario(valor_a_escribir_fs, *tam_esc_fs);
			send_fin_escritura(cliente_socket);
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

int server_escuchar() {
	server_name = "Memoria";
	int cliente_socket = esperar_cliente(logger, server_name, fd_memoria);

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
	tamanio_total=0;
	for(int i = 0; i < list_size(huecos_libres); i++){
		t_hueco_memoria* hueco_libre = list_get(huecos_libres, i);
		log_info(logger,"Queda un hueco libre en %d de tamaño %d", hueco_libre->base, hueco_libre->tamanio);
		if(hueco_libre->tamanio >= tamanio){
			return SEGMENT_CREATED;
		}
		tamanio_total+=hueco_libre->tamanio;
	}
	log_info(logger,"Memoria restante es Tamaño: %d",tamanio_total);
	if(tamanio_total>=tamanio)
		return COMPACT;
	return OUT_OF_MEM;
}

void inicializar_memoria(){
	log_info(logger, "Creando espacio de memoria...");
	espacio_usuario = malloc(TAM_MEMORIA);

	lista_ts_wrappers = list_create();
	huecos_libres = list_create();
	segmentos_en_memoria = list_create();

	segmento_0 = malloc(sizeof(t_segmento));
	segmento_0->base = 0;
	segmento_0->id = 0;
	segmento_0->tamanio = TAM_SEGMENTO_0;
	list_add(segmentos_en_memoria, segmento_0);
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

	return tabla_segmentos_inicial;
}

void terminar_proceso(int pid){
	eliminar_tabla_segmentos(pid);
}

void eliminar_tabla_segmentos(int pid){
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
	t_segmento* nuevo_segmento = crear_segmento(pid, id, hueco->base, tamanio);
	actualizar_hueco_libre(nuevo_segmento, hueco);
	return nuevo_segmento->base;
}

t_hueco_memoria* encontrar_hueco_first(int tamanio){
	for(int i = 0; i < list_size(huecos_libres); i++){
		t_hueco_memoria* hueco_libre = list_get(huecos_libres, i);
		if(hueco_libre->tamanio >= tamanio){
			return hueco_libre;
		}
	}
	return NULL;
}

t_hueco_memoria* encontrar_hueco_best(int tamanio){
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

t_hueco_memoria* encontrar_hueco_worst(int tamanio){
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
	list_add(segmentos_en_memoria, segmento);

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
	for(int i = 0; i < list_size(lista_ts_wrappers); i++){
		ts_wrapper *ts_proceso = list_get(lista_ts_wrappers, i);
		if(ts_proceso->pid==pid){
			list_add(ts_proceso->tabla_de_segmentos,segmento);
		}
	}
	return;
}

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
					list_remove_element(segmentos_en_memoria,segmento);
					log_info(logger_obligatorio, "PID: %d - Eliminar Segmento: %d - Base: %d - TAMAÑO: %d", pid, id_segmento, base, tamanio);
				}
			}
		}
	}
	log_segmentos_en_memoria();
	agregar_hueco_libre(base, tamanio);
	return ts_actualizada;
}

void agregar_hueco_libre(int base, int tamanio){
    t_hueco_memoria *aux = NULL;
    int i;
    int numero = 0;
    t_hueco_memoria* hueco_nuevo = malloc(sizeof(t_hueco_memoria));
    hueco_nuevo -> base = base;
    hueco_nuevo -> tamanio = tamanio;

    for(i=0 ; i < list_size(huecos_libres); i++){
        t_hueco_memoria *segmento = list_get(huecos_libres, i);
        if(segmento->base < base){
            aux = segmento;
            numero = i+1;
        }
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
            list_add_in_index(huecos_libres,numero,hueco_nuevo);
            aux = hueco_nuevo;
        }
    }
    for(i=0; i<list_size(huecos_libres);i++){
        t_hueco_memoria *segmento =list_get(huecos_libres,i);
        if(aux->base< segmento->base && aux->base + aux -> tamanio == segmento->base){
            aux->tamanio +=segmento->tamanio;
            list_remove(huecos_libres,i);
        }
    }

    for(i=0 ; i < list_size(huecos_libres); i++){
        t_hueco_memoria *segmento = list_get(huecos_libres, i);
        log_info(logger, "Hueco libre n: %d base: %d tamanio: %d\t", i , segmento->base, segmento->tamanio);
        }
}


void compactar(){
	log_segmentos_en_memoria();
	list_sort(segmentos_en_memoria, (void*) comparador_de_base);
	log_segmentos_en_memoria();
	int tam_segmento=0;
	int base_segmento=0;
	for(int i = 0; i < list_size(segmentos_en_memoria); i++){
		t_segmento* segmento_actual = list_get(segmentos_en_memoria, i);
		int old_base = segmento_actual->base;
		log_info(logger,"segmento OLD base: %d", old_base);
		int new_base = base_segmento+tam_segmento;

		// Muevo lo que estaba escrito en espacio usuario junto con el segmento
		memcpy(espacio_usuario + new_base, espacio_usuario + segmento_actual->base, segmento_actual->tamanio);

		segmento_actual->base=new_base;
		log_info(logger,"segmento NEW base: %d", segmento_actual->base);
		base_segmento=segmento_actual->base;
		tam_segmento=segmento_actual->tamanio;

		//buscar segmento por base desactualizada y no por id pq hay colision
		//actualizar_segmento(old_base, new_base);
	}
	int cant_huecos = list_size(huecos_libres);
	log_info(logger, "cantidad de huecos libres que quedaron de la compactacion: %d", cant_huecos);
	for(int i = 0; i < cant_huecos; i++) {
		list_remove(huecos_libres, 0);
	}
//	log_info(logger, "se borraron todos los huecos libres y ahora quedan %d en la lista", list_size(huecos_libres));
	t_hueco_memoria* hueco_nuevo = malloc(sizeof(t_hueco_memoria));
	hueco_nuevo -> base = base_segmento+tam_segmento;
	hueco_nuevo -> tamanio = tamanio_total;

//  esto se encarga de sobreescribir la basura con null
//	char* vacio = malloc(hueco_nuevo->tamanio);
//	memcpy(espacio_usuario + hueco_nuevo->base, vacio, hueco_nuevo->tamanio);

	log_info(logger,"tam: %d", tamanio_total);
	list_add(huecos_libres,hueco_nuevo);
	return;
}

void actualizar_segmento(int old_base, int new_base){
	for(int i = 0; i < list_size(lista_ts_wrappers); i++){
		ts_wrapper* wrapper = list_get(lista_ts_wrappers, i);
		if(buscar_segmento_en_ts(old_base, new_base, wrapper->tabla_de_segmentos)){
			return;
		}
	}
	log_info(logger, "no se encontro el segmento de old_base: %d para actualizar el ts wrapper", old_base);
}

bool buscar_segmento_en_ts(int old_base, int new_base, t_list* tabla_segmentos){
	for(int i = 0; i < list_size(tabla_segmentos); i++){
		t_segmento* segmento_en_ts = list_get(tabla_segmentos, i);
		if(segmento_en_ts->base == old_base){
			segmento_en_ts->base = new_base;
			return true;
		}
	}
	return false;
}

bool comparador_de_base(t_segmento *s1, t_segmento *s2){
	return s1->base <= s2->base;
}

void log_resultado_compactacion(){
	// por cada segmento de cada proceso se debera imprimir una linea con el siguiente formato:
	// “PID: <PID> - Segmento: <ID SEGMENTO> - Base: <BASE> - Tamaño <TAMAÑO>”
	for(int i = 0; i < list_size(lista_ts_wrappers); i++){
		ts_wrapper* wrapper = list_get(lista_ts_wrappers, i);
		log_ts_de_pid(logger_obligatorio, wrapper->pid, wrapper->tabla_de_segmentos);
	}
}

void log_valor_espacio_usuario(char* valor, int tamanio){
	char* valor_log = malloc(tamanio);
	memcpy(valor_log, valor, tamanio);
	memcpy(valor_log + tamanio, "\0", 1);
	int tamanio_valor = strlen(valor_log);
	log_info(logger, "se leyo/escribio %s de tamaño %d en el espacio de usuario", valor_log, tamanio_valor);
}

void log_segmentos_en_memoria(){
	for(int i = 0; i < list_size(segmentos_en_memoria); i++){
		t_segmento* segmento = list_get(segmentos_en_memoria, i);
		log_info(logger, "Segmento en memoria: Id - %d, Base - %d, Tamaño - %d", segmento->id, segmento->base, segmento->tamanio);
	}
}





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
	inicializar_variables();
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
	TAM_MEMORIA = config_get_string_value(config, "TAM_MEMORIA");
	TAM_SEGMENTO_0 = config_get_string_value(config, "TAM_SEGMENTO_0");
	CANT_SEGMENTOS = config_get_string_value(config, "CANT_SEGMENTOS");
	RETARDO_MEMORIA = config_get_string_value(config, "RETARDO_MEMORIA");
	RETARDO_COMPACTACION = config_get_string_value(config, "RETARDO_COMPACTACION");
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

void inicializar_variables(){
	lista_ts_wrappers = list_create();
}

// --------------------- COMUNICACION ---------------------

static void procesar_conexion(void *void_args) {
	int *args = (int*) void_args;
	int cliente_socket = *args;

	op_code cop;
	while (cliente_socket != -1) {
		if (recv(cliente_socket, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
			log_info(logger, "El cliente se desconecto de %s server",
					server_name);
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
			t_list* create_sgm_params = recv_create_segment(fd_kernel);
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

	pthread_mutex_init(&memoria_usuario_mutex, NULL);

	log_info(logger, "Creando espacio de memoria...");
	espacio_memoria = malloc(atoi(TAM_MEMORIA));	//Le estaban pasando un char*, con atoi obtienen lo que desean

}

void inicializar_tabla_segmento(int id_proceso){
	t_segmentoss* segmento_0 = malloc(sizeof(t_segmentoss));					//TODO cambien el nombre de t_segmentoss, puse eso como provisorio
	segmento_0->base=0;								//No entiendo por qué es tipo void*, chequeen si eso es correcto
	segmento_0->tamano= atoi(TAM_SEGMENTO_0);		//TODO Estaban poniendo un char* en int, atoi les corrige eso
	t_tabla_segmento* tabla = malloc(sizeof(t_tabla_segmento));
	tabla->numSegmentos=1;
	tabla->id=id_proceso;
	t_list* lista_t_segmento = list_create();
	list_add(tabla->lista_segmentos,segmento_0);	//TODO Pasar bien los parámetros porque no le gusta así
    list_add(lista_t_segmento, tabla);				//TODO Inicialicé con punteros segmento_0 y tabla, chequeen los tipos de datos que reciben las funciones
}

// creamos lista por proceso que a su vez tiene segmentos
// cada proceso agregarle segmento_0

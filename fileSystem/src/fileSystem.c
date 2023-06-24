#include "../include/fileSystem.h"

int main(int argc, char **argv) {
	if (argc > 2) {
		return EXIT_FAILURE;
	}
	logger = log_create("filesystem.log", "filesystem_main", 1, LOG_LEVEL_INFO);
	config = config_create(argv[1]);
	if(config == NULL){
		log_error(logger, "No se encontró el archivo :(");
		exit(1);
	}

	inicializar_variables();
	iniciar_atencion_peticiones();

	// Conecto CPU con memoria
	fd_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
	enviar_mensaje("Hola, soy Filesystem.", fd_memoria);

	// Inicio de servidor
	fd_filesystem = iniciar_servidor(logger, IP, PUERTO);

	// Conexion Kernel
	pthread_t conexion_memoria;
	pthread_create(&conexion_memoria, NULL, (void*) server_escuchar, NULL);
	pthread_join(conexion_memoria, NULL);

	terminar_programa();
	return 0;
}

void leer_config(){
	IP = config_get_string_value(config, "IP_ESCUCHA");
	PUERTO = config_get_string_value(config, "PUERTO_ESCUCHA");
	IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
	PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");
	PATH_SUPERBLOQUE = config_get_string_value(config, "PATH_SUPERBLOQUE");
	PATH_BITMAP = config_get_string_value(config, "PATH_BITMAP");
	PATH_BLOQUES = config_get_string_value(config, "PATH_BLOQUES");
	PATH_FCB = config_get_string_value(config, "PATH_FCB");
	RETARDO_ACCESO_BLOQUE = config_get_string_value(config, "RETARDO_ACCESO_BLOQUE");
}

// ------------------ INIT --------------------------

void inicializar_variables(){
	leer_config();
	levantar_archivos();
	lista_fcbs = list_create();
	peticiones_pendientes = list_create();
	pthread_mutex_init(&mutex_peticiones_pendientes, NULL);
	sem_init(&contador_peticiones, 0, 0);
	inicializar_fcbs();
}

void inicializar_fcbs(){

	DIR *directorio_archivos = opendir(PATH_FCB);
	struct dirent *fcb;

	if(directorio_archivos == NULL){
		log_error(logger, "No se pudo abrir el directorio de fcbs");
		exit(1);
	}

	while((fcb = readdir(directorio_archivos)) != NULL){
		if (strcmp(fcb->d_name, ".") == 0 || strcmp(fcb->d_name, "..") == 0){
			continue;
		}
		log_info(logger, "Lei esto del directorio: %s", fcb->d_name);

		t_archivo *archivo = malloc(sizeof(t_archivo));
		archivo->nombre_archivo = malloc(strlen(fcb->d_name));
		strcpy(archivo->nombre_archivo, fcb->d_name);

		char* path_archivo = malloc(strlen(PATH_FCB) + strlen(fcb->d_name));
		strcpy(path_archivo, PATH_FCB);
		strcat(path_archivo, fcb->d_name);
		archivo->archivo_fcb = config_create(path_archivo);

		list_add(lista_fcbs, archivo);
	}

	closedir(directorio_archivos);

}

void leer_superbloque(){
	superbloque = config_create(PATH_SUPERBLOQUE);

	if(superbloque == NULL){
		log_error(logger, "No se encontró el archivo superbloque >:(");
		exit(1);
	}

	BLOCK_SIZE = config_get_int_value(superbloque, "BLOCK_SIZE");
	BLOCK_COUNT = config_get_int_value(superbloque, "BLOCK_COUNT");

	config_destroy(superbloque);

	// TODO comment de tomy: acuerdense de hacerle un config_destroy a superbloque si no lo usan mas
}

void crear_bitmap(){
	int fd;
	tamanio_bitmap = ceil(BLOCK_COUNT/8);

	if(access(PATH_BITMAP, F_OK) == -1){
		log_error(logger, "No pude acceder al archivo bitmap");
		exit(1);
	}

	fd = open(PATH_BITMAP, O_RDWR);
	buffer_bitmap = mmap(NULL, tamanio_bitmap, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	bitmap = bitarray_create_with_mode(buffer_bitmap, tamanio_bitmap, LSB_FIRST);

	// Esta parte puede que ni vaya por el momento
    // Copia los bits del bitarray al mapeo en memoria
//    memcpy(mmap_funciono, bitmap->bitarray, tamanio_bitmap);
//	//free(bitarray_p);
//
//    // Liberar recursos
//    munmap(mmap_funciono, tamanio_bitmap);
//    bitarray_destroy(bitmap);

	// TODO comment de tomy: tiene sentido cerrar el fd en la funcion que lo abre?
	// Investiguen si mmap puede syncear memoria con el archivo si el fd esta cerrado
	close(fd);

}

void crear_archivo_de_bloques(){
	int fd = open(PATH_BLOQUES, O_CREAT | O_RDWR);
	TAMANIO_ARCHIVO_BLOQUES = BLOCK_SIZE * BLOCK_COUNT;
	buffer_bloques = mmap(NULL, TAMANIO_ARCHIVO_BLOQUES, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if(fd == -1){
		log_error(logger, "Hubo un problema creando o abriendo el archivo de bloques >:(");
		exit(1);
	}

	// TODO comment de tomy: tiene sentido cerrar el fd en la funcion que lo abre?
	// Investiguen si mmap puede syncear memoria con el archivo si el fd esta cerrado
	close(fd);
}


void levantar_archivos(){
	leer_superbloque();
	crear_bitmap();
	crear_archivo_de_bloques();
}

void terminar_programa(){
	log_destroy(logger);
	config_destroy(config);
}

// ------------------ COMUNICACION ------------------

static void procesar_conexion() {
	op_code cop;

	while (socket_cliente != -1) {
        if (recv(socket_cliente, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
            log_info(logger, "El cliente se desconecto de %s server", server_name);
            return;
        }
		switch (cop) {
		case MENSAJE:
			recibir_mensaje(logger, socket_cliente);
			break;
		case MANEJAR_F_OPEN:
			char* nombre_archivo_open = recv_manejo_f_open(socket_cliente);
			log_info(logger,"Se esta ejecutando un MANEJAR_F_OPEN");
			t_peticion* peticion_open = crear_peticion(OPEN,nombre_archivo_open, 0, 0);
			agrego_a_pendientes(peticion_open);
			sem_post(&contador_peticiones);
			break;
		case MANEJAR_F_CREATE:
//			nombre_archivo = recv_manejo_f_open(socket_cliente);
//			log_info(logger,"Se esta ejecutando un MANEJAR_F_CREATE");
//			manejar_f_create(nombre_archivo);
			break;
		case MANEJAR_F_TRUNCATE:
			t_list* parametros_truncate = recv_manejo_f_truncate(socket_cliente);
			char* nombre_archivo_truncate = list_get(parametros_truncate, 0);
			int* tamanio_truncate = list_get(parametros_truncate, 1);
			log_info(logger,"Se esta ejecutando un MANEJAR_F_TRUNCATE");
			t_peticion* peticion_truncate = crear_peticion(TRUNCATE, nombre_archivo_truncate, *tamanio_truncate, 0);
			agrego_a_pendientes(peticion_truncate);
			sem_post(&contador_peticiones);
			break;
		case MANEJAR_F_READ:
			t_list* parametros_read = recv_manejo_f_read(socket_cliente);
			char* nombre_archivo_read = list_get(parametros_read, 0);
			int* tamanio_read = list_get(parametros_read, 1);
			int* dir_fisica_read = list_get(parametros_read, 2);

			log_info(logger,"Se esta ejecutando un MANEJAR_F_READ");
			t_peticion* peticion_read = crear_peticion(READ, nombre_archivo_read, *tamanio_read, *dir_fisica_read);
			agrego_a_pendientes(peticion_read);
			sem_post(&contador_peticiones);
			break;
		case MANEJAR_F_WRITE:
			t_list* parametros_write = recv_manejo_f_write(socket_cliente);
			char* nombre_archivo_write = list_get(parametros_write, 0);
			int* dir_fisica_write = list_get(parametros_write, 1);
			int* tamanio_write = list_get(parametros_write, 2);

			log_info(logger,"Se esta ejecutando un MANEJAR_F_WRITE");
			t_peticion* peticion_write = crear_peticion(WRITE, nombre_archivo_write, *tamanio_write, *dir_fisica_write);
			agrego_a_pendientes(peticion_write);
			sem_post(&contador_peticiones);
			break;
		default:
			log_error(logger, "Algo anduvo mal en el server de %s", server_name);
			return;
		}
	}

	log_warning(logger, "El cliente se desconecto de %s server", server_name);
	return;
}

void server_escuchar() {
	server_name = "Filesystem";
	socket_cliente = esperar_cliente(logger, server_name, fd_filesystem);

	if (socket_cliente == -1){
		log_info(logger, "Hubo un error en la conexion del %s", server_name);
	}
	procesar_conexion();
}


// ------------------ MANEJO OPERACIONES ------------------

void manejar_peticion(t_peticion* peticion){
	t_operacion_fs cop = peticion->operacion;

	switch (cop) {
	case OPEN:
		log_info(logger,"Se esta ejecutando un F_OPEN");
		log_info(logger,"Me llego este nombre: %s", peticion->nombre);
		manejar_f_open(peticion->nombre);
		send_fin_f_open(socket_cliente);
		break;
	case TRUNCATE:
		log_info(logger,"Se esta ejecutando un F_TRUNCATE");
		manejar_f_truncate(peticion->nombre, peticion->tamanio);
		send_fin_operacion_bloqueante(socket_cliente);
		break;
	case READ:
		log_info(logger,"Se esta ejecutando un F_READ");
		manejar_f_read(peticion->nombre, peticion->dir_fisica, peticion->tamanio);
		send_fin_operacion_bloqueante(socket_cliente);
		break;
	case WRITE:
		log_info(logger,"Se esta ejecutando un F_WRITE");
		manejar_f_write(peticion->nombre, peticion->dir_fisica, peticion->tamanio);
		send_fin_operacion_bloqueante(socket_cliente);
		break;
	default:
		log_error(logger, "Algo anduvo mal en el server de %s", server_name);
		return;
	}
}

t_peticion* crear_peticion(t_operacion_fs operacion, char* nombre, int tamanio, int dir_fisica){
	t_peticion* peticion = malloc(sizeof(t_peticion));
	peticion->nombre = malloc(strlen(nombre) + 1);

	peticion->operacion = operacion;
	strcpy(peticion->nombre, nombre);
	peticion->tamanio = tamanio;
	peticion->dir_fisica = dir_fisica;

	return peticion;
}

void agrego_a_pendientes(t_peticion* peticion){
	pthread_mutex_lock(&mutex_peticiones_pendientes);
	list_add(peticiones_pendientes, peticion);
	pthread_mutex_unlock(&mutex_peticiones_pendientes);
}

t_peticion* saco_de_pendientes(){
	pthread_mutex_lock(&mutex_peticiones_pendientes);
	t_peticion* peticion = list_remove(peticiones_pendientes, 0);
	pthread_mutex_unlock(&mutex_peticiones_pendientes);
	return peticion;
}

void iniciar_atencion_peticiones(){
	pthread_t hilo_peticiones;
	log_info(logger, "Inicio atencion de peticiones");

	pthread_create(&hilo_peticiones, NULL, (void*) atender_peticiones, NULL);
	pthread_detach(hilo_peticiones);
}

void atender_peticiones(){
	while(1){
		sem_wait(&contador_peticiones);
		log_info(logger, "Hay peticion pendiente");
		t_peticion* peticion = saco_de_pendientes();
		manejar_peticion(peticion);
	}
}

bool existe_fcb(char* nombre_archivo){
	for(int i = 0; i < list_size(lista_fcbs); i++){
		t_archivo* archivo_buscado = list_get(lista_fcbs, i);

		if(strcmp(archivo_buscado->nombre_archivo, nombre_archivo) == 0){
			return true;
		}
	}
	return false;
}

t_config* obtener_archivo(char* nombre_archivo){
	for(int i = 0; i < list_size(lista_fcbs); i++){
		t_archivo* archivo_buscado = list_get(lista_fcbs, i);

		if(strcmp(archivo_buscado->nombre_archivo, nombre_archivo) == 0){
			return archivo_buscado->archivo_fcb;
		}
	}
	return NULL;
}

void manejar_f_open(char* nombre_archivo){
	if(!existe_fcb(nombre_archivo)){
		//send_aviso_archivo_inexistente(socket_cliente);
		manejar_f_create(nombre_archivo);
	}
	//send_confirmacion_archivo_abierto(socket_cliente);
}

void manejar_f_create(char* nombre_archivo){
	char* path_archivo = malloc(strlen(PATH_FCB) + strlen(nombre_archivo));
	strcpy(path_archivo, PATH_FCB);
	strcat(path_archivo, nombre_archivo);

	t_fcb *nuevo_fcb = malloc(sizeof(t_fcb));
	nuevo_fcb->nombre_archivo = malloc(strlen(nombre_archivo));
	strcpy(nuevo_fcb->nombre_archivo, nombre_archivo);
	nuevo_fcb->tamanio_archivo = 0;
	nuevo_fcb->puntero_directo = buscar_bloque_libre();
	nuevo_fcb->puntero_indirecto = buscar_bloque_libre();
	char* text_tamanio_archivo = malloc(10);
	char* text_puntero_directo = malloc(10);
	char* text_puntero_indirecto = malloc(10);

	sprintf(text_tamanio_archivo, "%d", nuevo_fcb->tamanio_archivo);
	sprintf(text_puntero_directo, "%d", nuevo_fcb->puntero_directo);
	sprintf(text_puntero_indirecto, "%d", nuevo_fcb->puntero_indirecto);

	FILE* f_fcb = fopen(path_archivo, "a+");

	t_archivo *archivo_fcb = malloc(sizeof(t_archivo));
	archivo_fcb->nombre_archivo = malloc(strlen(nombre_archivo));
	archivo_fcb->archivo_fcb = config_create(path_archivo);

	config_set_value(archivo_fcb->archivo_fcb, "NOMBRE_ARCHIVO", nuevo_fcb->nombre_archivo);
	config_set_value(archivo_fcb->archivo_fcb, "TAMANIO_ARCHIVO", text_tamanio_archivo);
	config_set_value(archivo_fcb->archivo_fcb, "PUNTERO_DIRECTO", text_puntero_directo);
	config_set_value(archivo_fcb->archivo_fcb, "PUNTERO_INDIRECTO", text_puntero_indirecto);
	config_save(archivo_fcb->archivo_fcb);

	list_add(lista_fcbs, archivo_fcb);
	fclose(f_fcb);
}

int obtener_cantidad_punteros(uint32_t* array_punteros){
	//ver q onda con lo q nos dice nahu maniana
	return 0;
}

uint32_t buscar_bloque_libre(){ //busca un bloque libre y lo ocupa

	for(int i = 0; i < tamanio_bitmap; i++){
		if(!bitarray_test_bit(bitmap, i)){
			bitarray_set_bit(bitmap, i);
			return i;
		}
	}

	log_error(logger, "No hay bloques libres");
	return 0;
}

void asignar_bloques(int cant_bloques, t_config* archivo){
	int puntero_indirecto = config_get_int_value(archivo, "PUNTERO_INDIRECTO");

	uint32_t* array_bloque_de_punteros = malloc(BLOCK_SIZE);
	int pos_bloque_punteros = puntero_indirecto*BLOCK_SIZE;
	memcpy(array_bloque_de_punteros, buffer_bloques + pos_bloque_punteros, BLOCK_SIZE);

	int cant_punteros_bloque = obtener_cantidad_punteros(array_bloque_de_punteros);
	int pos_nuevo_bloque = cant_punteros_bloque*sizeof(uint32_t);

	for(int i = cant_bloques; i > 0; i--){
		uint32_t puntero_a_bloque = buscar_bloque_libre();
		memcpy(buffer_bloques+pos_bloque_punteros+pos_nuevo_bloque, &puntero_a_bloque, sizeof(uint32_t));
		pos_nuevo_bloque += sizeof(uint32_t);
	}

}

void sacar_bloques(int cant_bloques, t_config* archivo){
	int puntero_indirecto = config_get_int_value(archivo, "PUNTERO_INDIRECTO");

	uint32_t* array_bloque_de_punteros = malloc(BLOCK_SIZE);
	int pos_bloque_punteros = puntero_indirecto*BLOCK_SIZE;
	memcpy(array_bloque_de_punteros, buffer_bloques + pos_bloque_punteros, BLOCK_SIZE);

	int cant_punteros_bloque = obtener_cantidad_punteros(array_bloque_de_punteros);
	int pos_ultimo_bloque = (cant_punteros_bloque-1)*sizeof(uint32_t);
	int pos_bitmap_ultimo_bloque = cant_punteros_bloque - 1;

	for(int i = cant_bloques; i > 0; i--){
		//liberar_bloque(pos_ultimo_bloque); TODO: esta funcion tiene q poner en el valor q nos diga nahu al bloque en el archivo de bloques, basicamente borrar el puntero
		bitarray_clean_bit(bitmap, pos_bitmap_ultimo_bloque);
		pos_ultimo_bloque -= sizeof(uint32_t);
		pos_bitmap_ultimo_bloque--;
	}

}

void manejar_f_truncate(char* nombre_archivo, int tamanio_nuevo){
	t_config* archivo_fcb = obtener_archivo(nombre_archivo);
	int tamanio_fcb = config_get_int_value(archivo_fcb, "TAMANIO_ARCHIVO");
	char* texto_tamanio_archivo = malloc(10);
	sprintf(texto_tamanio_archivo, "%d", tamanio_nuevo);

	if(tamanio_nuevo > tamanio_fcb){
		// AMPLIAR
		config_set_value(archivo_fcb, "TAMANIO_ARCHIVO", texto_tamanio_archivo);
		int cantidad_bloques_a_agregar = tamanio_nuevo - tamanio_fcb;
		//asignar_bloques(cantidad_bloques_a_agregar, archivo_fcb);
	} else{
		// REDUCIR
		config_set_value(archivo_fcb, "TAMANIO_ARCHIVO", texto_tamanio_archivo);
		int cantidad_bloques_a_sacar = tamanio_fcb - tamanio_nuevo;
		//sacar_bloques(cantidad_bloques_a_sacar, archivo_fcb);
	}
}

void manejar_f_read(char* nombre_archivo, int dir_fisica, int tamanio){

}

void manejar_f_write(char* nombre_archivo, int dir_fisica, int tamanio){

}

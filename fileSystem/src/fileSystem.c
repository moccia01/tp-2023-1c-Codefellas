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
	int tamanio_bitmap = ceil(BLOCK_COUNT/8);

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
			log_info(logger,"Me llego este nombre: %s", nombre_archivo_open);
			manejar_f_open(nombre_archivo_open);
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
			manejar_f_truncate(nombre_archivo_truncate, *tamanio_truncate);
			log_info(logger,"Se esta ejecutando un MANEJAR_F_TRUNCATE");
			break;
		case MANEJAR_F_READ:
			t_list* parametros_read = recv_manejo_f_read(socket_cliente);
			char* nombre_archivo_read = list_get(parametros_read, 0);
			int* tamanio_read = list_get(parametros_read, 1);
			int* dir_fisica_read = list_get(parametros_read, 2);
			manejar_f_read(nombre_archivo_read, *dir_fisica_read, *tamanio_read);
			log_info(logger,"Se esta ejecutando un MANEJAR_F_READ");
			break;
		case MANEJAR_F_WRITE:
			t_list* parametros_write = recv_manejo_f_write(socket_cliente);
			char* nombre_archivo_write = list_get(parametros_write, 0);
			int* dir_fisica_write = list_get(parametros_write, 1);
			int* tamanio_write = list_get(parametros_write, 2);
			manejar_f_write(nombre_archivo_write, *dir_fisica_write, *tamanio_write);
			log_info(logger,"Se esta ejecutando un MANEJAR_F_WRITE");
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
	nuevo_fcb->puntero_directo = 0;
	nuevo_fcb->puntero_indirecto = 0;

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
	send_confirmacion_archivo_creado(socket_cliente);
	fclose(f_fcb);
}

void manejar_f_truncate(char* nombre_archivo, int tamanio){
	t_config* archivo_fcb = obtener_archivo(nombre_archivo);
	int tamanio_fcb = config_get_int_value(archivo_fcb, "TAMANIO");

	if(tamanio > tamanio_fcb){
		// AMPLIAR
	} else{
		// REDUCIR
	}
}

void manejar_f_read(char* nombre_archivo, int dir_fisica, int tamanio){

}

void manejar_f_write(char* nombre_archivo, int dir_fisica, int tamanio){

}

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
	leer_config();

	levantar_archivos();
	//inicializar_variables();

	// Conecto CPU con memoria
	fd_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
	enviar_mensaje("Hola, soy Filesystem.", fd_memoria);

	// Inicio de servidor
	fd_filesystem = iniciar_servidor(logger, IP, PUERTO);

	// Conexion Kernel
	pthread_t conexion_memoria;
	pthread_create(&conexion_memoria, NULL, (void*) server_escuchar, NULL);
	pthread_join(conexion_memoria, NULL);

	terminar_programa(logger, config);
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
}

// ------------------ INIT --------------------------

void inicializar_variables(){
	//ARRAY_BLOQUES[TAMANIO];
	//ARRAY_BITMAP[BLOCK_COUNT];
}

void leer_superbloque(){

	superbloque = config_create(PATH_SUPERBLOQUE);

	if(superbloque == NULL){
		log_error(logger, "No se encontró el archivo superbloque >:(");
		exit(1);
	}

	BLOCK_SIZE = config_get_int_value(superbloque, "BLOCK_SIZE");
	BLOCK_COUNT = config_get_int_value(superbloque, "BLOCK_COUNT");
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
    close(fd);

}

void crear_archivo_de_bloques(){

	//Primero deberia leer el archivo y en caso de que no este creado crearlo
	int fd = open(PATH_BLOQUES, O_CREAT | O_RDWR);
	TAMANIO = BLOCK_SIZE * BLOCK_COUNT;
	buffer_bloques = mmap(NULL, TAMANIO, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if(fd == -1){
		log_error(logger, "Hubo un problema creando o abriendo el archivo de bloques >:(");
		exit(1);
	}

	//X AHORA QUEDA HASTA ACA CHECKPOINT 3

	/*

	//Ver que onda despues con esto
	TAMANIO = BLOCK_SIZE * BLOCK_COUNT;
	ARRAY_BLOQUES= malloc(TAMANIO);

	for(int i = 0; i < TAMANIO; i++){
		//ARRAY_BLOQUES[i] = malloc();
		strcpy(ARRAY_BLOQUES[i], "");
	}
	*/
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
	char* nombre_archivo;
	int* tamanio;
	int* dir_fisica;
	t_list* parametros_fs;

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
			parametros_fs = recv_elementos_fs(socket_cliente);	//TODO Chequear esta funcion, parece que rompe
			nombre_archivo = list_get(parametros_fs, 0);
			log_info(logger,"Se esta ejecutando un MANEJAR_F_OPEN");
			manejar_f_open(nombre_archivo);
			break;
		case MANEJAR_F_CREATE:
			parametros_fs = recv_elementos_fs(socket_cliente);
			nombre_archivo = list_get(parametros_fs, 0);
			log_info(logger,"Se esta ejecutando un MANEJAR_F_CREATE");
			manejar_f_create(nombre_archivo);
			break;
		case MANEJAR_F_TRUNCATE:
			parametros_fs = recv_elementos_fs(socket_cliente);
			nombre_archivo = list_get(parametros_fs, 0);
			tamanio = list_get(parametros_fs, 1);
			manejar_f_truncate(nombre_archivo, tamanio);
			log_info(logger,"Se esta ejecutando un MANEJAR_F_TRUNCATE");
			break;
		case MANEJAR_F_READ:
			parametros_fs = recv_elementos_fs(socket_cliente);
			nombre_archivo = list_get(parametros_fs, 0);
			tamanio = list_get(parametros_fs, 1);
			dir_fisica = list_get(parametros_fs, 2);
			manejar_f_read(nombre_archivo, dir_fisica, tamanio);
			log_info(logger,"Se esta ejecutando un MANEJAR_F_READ");
			break;
		case MANEJAR_F_WRITE:
			parametros_fs = recv_elementos_fs(socket_cliente);
			nombre_archivo = list_get(parametros_fs, 0);
			tamanio = list_get(parametros_fs, 1);
			dir_fisica = list_get(parametros_fs, 2);
			manejar_f_write(nombre_archivo, dir_fisica, tamanio);
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

	if (socket_cliente == -1) {
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
		send_aviso_archivo_inexistente(socket_cliente);
		manejar_f_create(nombre_archivo);
	}
	send_confirmacion_archivo_abierto(socket_cliente);
}

void manejar_f_create(char* nombre_archivo){

	char* PATH_FCB = malloc(strlen(nombre_archivo));
	strcpy(PATH_FCB,"/home/utnso/tp-2023-1c-Codefellas/fileSystem/archivos/fcbs/");

	fcb *nuevo_fcb = malloc(sizeof(fcb));
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

	t_archivo *archivo_fcb = malloc(sizeof(t_archivo));
	archivo_fcb->nombre_archivo = malloc(strlen(nombre_archivo));
	archivo_fcb->archivo_fcb = config_create(strcat(PATH_FCB, nombre_archivo));

	config_set_value(archivo_fcb->archivo_fcb, "NOMBRE_ARCHIVO", nuevo_fcb->nombre_archivo);
	config_set_value(archivo_fcb->archivo_fcb, "TAMANIO_ARCHIVO", text_tamanio_archivo);
	config_set_value(archivo_fcb->archivo_fcb, "PUNTERO_DIRECTO", text_puntero_directo);
	config_set_value(archivo_fcb->archivo_fcb, "PUNTERO_INDIRECTO", text_puntero_indirecto);

	list_add(lista_fcbs, archivo_fcb);
	//TODO Meter fcb a la lista de fcbs o crear un archivo con el nombre_archivo pasado y no tener un tipo de dato fcb
	send_confirmacion_archivo_creado(socket_cliente);
}

void manejar_f_truncate(char* nombre_archivo, int* tamanio){

	t_config* archivo_fcb = obtener_archivo(nombre_archivo);
	int tamanio_fcb = config_get_int_value(archivo_fcb, "TAMANIO");

	if(tamanio > &tamanio_fcb){
		// AMPLIAR
	} else{
		// REDUCIR
	}
}

void manejar_f_read(char* nombre_archivo, int* dir_fisica, int* tamanio){

}

void manejar_f_write(char* nombre_archivo, int* dir_fisica, int *tamanio){

}

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

	levantar_archivos();	//TODO Levantar los archivos de manera correcta, no basta con fopen https://linuxhint.com/using_mmap_function_linux/
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

//ESTA DE ABAJO YA ESTA TERMINADA
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

	//Primero deberia leer el archivo y en caso de que no este creado crearlo
	//Datos utiles, r+ abre un archivo EXISTENTE en read write, w+ crea un archivo y lo abre en read write

	FILE* fp;
	fp = fopen(PATH_BITMAP, "r+");

	if(fp == NULL){
		fp = fopen(PATH_BITMAP, "w+");
		if(fp == NULL){
			log_error(logger, "Hubo un problema creando o abriendo el archivo de bitmap >:(");
			exit(1);
		}
	}

	//Calculo el tamanio del bitarray del bitmap
	int tamanio_bitmap = ceil(BLOCK_COUNT / 8); //esto es xq nosotros nos manejamos con bytes
	void* bitarray = mmap(NULL, tamanio_bitmap, PROT_READ | PROT_WRITE, MAP_SHARED, fd_memoria, 0);
	bitmap = bitarray_create_with_mode(bitarray, tamanio_bitmap, LSB_FIRST);



/*
	int tamanio_bitmap = BLOCK_COUNT / 8;

	//mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
	void* bitarray = mmap(NULL, tamanio_bitmap, PROT_READ | PROT_WRITE, MAP_SHARED, fd_memoria, 0);
	//void* bitarray = malloc(tamanio_bitmap);
	bitmap = bitarray_create_with_mode(bitarray, tamanio_bitmap, LSB_FIRST);
	msync(bitmap->bitarray, tamanio_bitmap, MS_SYNC);
	log_info(logger, "Se guardó en msync");
	//bitarray_set_bit(bitmap, off_t bit_index);

	for(int i = 0; i < BLOCK_COUNT; i++){
		ARRAY_BITMAP[i] = 0;
	}
*/
}


void crear_archivo_de_bloques(){

	//Primero deberia leer el archivo y en caso de que no este creado crearlo
	FILE* fp;
	fp = fopen(PATH_BLOQUES, "r+");

	if(fp == NULL){
		fp = fopen(PATH_BLOQUES, "w+");
		if(fp == NULL){
			log_error(logger, "Hubo un problema creando o abriendo el archivo de bloques >:(");
			exit(1);
		}
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
			break;
		case MANEJAR_F_CREATE:
			break;
		case MANEJAR_F_TRUNCATE:
			break;
		case MANEJAR_F_READ:
			break;
		case MANEJAR_F_WRITE:
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

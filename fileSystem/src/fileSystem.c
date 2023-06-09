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

	//Primero deberia leer el archivo y en caso de que no este creado crearlo
	int fd;

	if(access(PATH_BITMAP, F_OK) != -1){
		fd = open(PATH_BITMAP, O_RDWR);
	}
	else{
		fd = -1;

	}

	if(fd == -1){
		// El archivo no existe o no se pudo abrir, se puede crear durante la ejecución
		fd = open(PATH_BITMAP, O_CREAT, S_IRUSR | S_IWUSR);

		if(fd == -1){
			log_error(logger, "Hubo un problema creando o abriendo el archivo de bitmap >:(");
			exit(1);
		}

		log_info(logger, "fd: %d", fd);
		//lleno de 0s el bitarray
		int tamanio_bitmap = ceil(BLOCK_COUNT/8.0);
		//int tamanio_bitmap = 8;
		char bitarray[tamanio_bitmap]; 				//TODO: asignar a tamanio bitmap un valor bajo para debbuguear y chequear si el resto funciona

		for(int i = 0; i < tamanio_bitmap; i++){
			bitarray[i] = '0'; // Cambiado a asignación de caracteres
		}

		char* bitarray_p = malloc((strlen(bitarray) + 1) * sizeof(char));

		strcpy(bitarray_p, bitarray);
		write(fd, bitarray, sizeof(bitarray));
		//write(fd, &bitarray_p, strlen(bitarray_p + 1));
		bitmap = bitarray_create_with_mode(bitarray_p, tamanio_bitmap, LSB_FIRST);
		void* mmap_funciono = mmap(NULL, tamanio_bitmap, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

		if(mmap_funciono == MAP_FAILED){
			log_error(logger, "Error ejecutando mmap. ");
			close(fd);
			return;
		}
		//free(bitarray_p);
	}

	close(fd);

}

void crear_archivo_de_bloques(){

	//Primero deberia leer el archivo y en caso de que no este creado crearlo
	int fd = open(PATH_BLOQUES, O_CREAT | O_RDWR);

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

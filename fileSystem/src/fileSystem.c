#include "../include/fileSystem.h"

int main(int argc, char **argv) {
	if (argc > 2) {
		return EXIT_FAILURE;
	}
	logger = log_create("filesystem.log", "filesystem_main", 1, LOG_LEVEL_INFO);
	logger_obligatorio = log_create("filesystem.log", "filesystem_obligatorio", 1, LOG_LEVEL_INFO);
	config = config_create(argv[1]);
	if(config == NULL){
		log_error(logger, "No se encontró el archivo :(");
		exit(1);
	}
	inicializar_variables();
	iniciar_atencion_peticiones();

	// Conecto CPU con memoria
	fd_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
	enviar_mensaje("Hola, soy Filesystem!", fd_memoria);

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
	RETARDO_ACCESO_BLOQUE = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE");
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

	// TODO comment de tomy: tiene sentido cerrar el fd en la funcion que lo abre?
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

	// TODO comment de tomy: tiene sentido cerrar el fd en la funcion que lo abre? 	Investiguen si mmap puede syncear memoria con el archivo si el fd esta cerrado
	close(fd);
}

void destruir_lista_fcbs(){
	list_destroy_and_destroy_elements(lista_fcbs, (void*) archivo_destroy);
	//list_destroy();
}

void archivo_destroy(t_archivo* archivo_fcb){
	config_destroy(archivo_fcb->archivo_fcb);
	free(archivo_fcb);
	archivo_fcb = NULL;
}

void levantar_archivos(){
	leer_superbloque();
	crear_bitmap();
	crear_archivo_de_bloques();
}

void terminar_programa(){
	log_destroy(logger);
	config_destroy(config);
	destruir_lista_fcbs();
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
			t_peticion* peticion_open = crear_peticion(OPEN,nombre_archivo_open, 0, 0, 0, 0);
			agrego_a_pendientes(peticion_open);
			sem_post(&contador_peticiones);
			break;
		case MANEJAR_F_TRUNCATE:
			t_list* parametros_truncate = recv_manejo_f_truncate(socket_cliente);
			char* nombre_archivo_truncate = list_get(parametros_truncate, 0);
			int* tamanio_truncate = list_get(parametros_truncate, 1);
			log_info(logger,"Se esta ejecutando un MANEJAR_F_TRUNCATE");
			t_peticion* peticion_truncate = crear_peticion(TRUNCATE, nombre_archivo_truncate, *tamanio_truncate, 0, 0, 0);
			agrego_a_pendientes(peticion_truncate);
			sem_post(&contador_peticiones);
			break;
		case MANEJAR_F_READ:
			t_list* parametros_read = recv_manejo_f_read_fs(socket_cliente);
			char* nombre_archivo_read = list_get(parametros_read, 0);
			int* dir_fisica_read = list_get(parametros_read, 1);
			int* tamanio_read = list_get(parametros_read, 2);
			int* posicion_a_leer = list_get(parametros_read, 3);
			int* pid_read = list_get(parametros_read, 4);

			log_info(logger,"Se esta ejecutando un MANEJAR_F_READ");
			t_peticion* peticion_read = crear_peticion(READ, nombre_archivo_read, *tamanio_read, *dir_fisica_read, *posicion_a_leer, *pid_read);
			agrego_a_pendientes(peticion_read);
			sem_post(&contador_peticiones);
			break;
		case MANEJAR_F_WRITE:
			t_list* parametros_write = recv_manejo_f_write_fs(socket_cliente);
			char* nombre_archivo_write = list_get(parametros_write, 0);
			int* dir_fisica_write = list_get(parametros_write, 1);
			int* tamanio_write = list_get(parametros_write, 2);
			int* posicion_a_escribir = list_get(parametros_write, 3);
			int* pid_write = list_get(parametros_write, 4);

			log_info(logger,"Se esta ejecutando un MANEJAR_F_WRITE");
			t_peticion* peticion_write = crear_peticion(WRITE, nombre_archivo_write, *tamanio_write, *dir_fisica_write, *posicion_a_escribir, *pid_write);
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
		send_fin_f_truncate(socket_cliente);
		break;
	case READ:
		log_info(logger,"Se esta ejecutando un F_READ");
		manejar_f_read(peticion->nombre, peticion->dir_fisica, peticion->tamanio, peticion->puntero, peticion->pid);
		send_fin_f_read(socket_cliente);
		break;
	case WRITE:
		log_info(logger,"Se esta ejecutando un F_WRITE");
		manejar_f_write(peticion->nombre, peticion->dir_fisica, peticion->tamanio, peticion->puntero, peticion->pid);
		send_fin_f_write(socket_cliente);
		break;
	default:
		log_error(logger, "Algo anduvo mal en el server de %s", server_name);
		return;
	}
}

t_peticion* crear_peticion(t_operacion_fs operacion, char* nombre, int tamanio, int dir_fisica, int puntero, int pid){
	t_peticion* peticion = malloc(sizeof(t_peticion));
	peticion->nombre = malloc(strlen(nombre) + 1);

	peticion->operacion = operacion;
	strcpy(peticion->nombre, nombre);
	peticion->tamanio = tamanio;
	peticion->dir_fisica = dir_fisica;
	peticion->puntero = puntero;
	peticion->pid = pid;

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
	log_info(logger_obligatorio, "Abrir Archivo: %s", nombre_archivo);
	if(!existe_fcb(nombre_archivo)){
		manejar_f_create(nombre_archivo);
	}
}

void manejar_f_create(char* nombre_archivo){
	log_info(logger_obligatorio, "Crear Archivo: %s", nombre_archivo);
	char* path_archivo = malloc(strlen(PATH_FCB) + strlen(nombre_archivo));
	strcpy(path_archivo, PATH_FCB);
	strcat(path_archivo, nombre_archivo);

	t_fcb *nuevo_fcb = malloc(sizeof(t_fcb));
	nuevo_fcb->nombre_archivo = malloc(strlen(nombre_archivo));
	strcpy(nuevo_fcb->nombre_archivo, nombre_archivo);
	nuevo_fcb->tamanio_archivo = 0;
	char* text_tamanio_archivo = malloc(10);
	sprintf(text_tamanio_archivo, "%d", nuevo_fcb->tamanio_archivo);

//	FILE* f_fcb = fopen(path_archivo, "a+");

	t_archivo *archivo_fcb = malloc(sizeof(t_archivo));
	archivo_fcb->nombre_archivo = malloc(strlen(nombre_archivo));
	strcpy(archivo_fcb->nombre_archivo, nombre_archivo);
	archivo_fcb->archivo_fcb = config_create(path_archivo);

	config_set_value(archivo_fcb->archivo_fcb, "NOMBRE_ARCHIVO", nuevo_fcb->nombre_archivo);
	config_set_value(archivo_fcb->archivo_fcb, "TAMANIO_ARCHIVO", text_tamanio_archivo);

	config_save(archivo_fcb->archivo_fcb);

	list_add(lista_fcbs, archivo_fcb);
//	fclose(f_fcb);
}

int obtener_cantidad_punteros(uint32_t* array_punteros){
	uint32_t* cant_punteros_bloque = malloc(sizeof(uint32_t));

	usleep(RETARDO_ACCESO_BLOQUE);
	memcpy(cant_punteros_bloque, array_punteros, sizeof(uint32_t));

	return *cant_punteros_bloque;
}

void liberar_bloque(int posicion_ultimo_bloque, uint32_t* array_bloque_de_punteros){
	uint32_t* nuevo_valor_puntero = malloc(sizeof(uint32_t));
	*nuevo_valor_puntero = 0;

	usleep(RETARDO_ACCESO_BLOQUE);
	//poner el ultimo puntero en 0
	memcpy(array_bloque_de_punteros + posicion_ultimo_bloque, nuevo_valor_puntero,sizeof(uint32_t));
}

uint32_t buscar_bloque_libre(){ //busca un bloque libre y lo ocupa
	for(int i = 0; i < tamanio_bitmap; i++){
		if(!bitarray_test_bit(bitmap, i)){
			bitarray_set_bit(bitmap, i);
			log_info(logger_obligatorio, "Acceso a Bitmap - Bloque: %d - Estado: 1", i);
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

	usleep(RETARDO_ACCESO_BLOQUE);
	memcpy(array_bloque_de_punteros, buffer_bloques + pos_bloque_punteros, BLOCK_SIZE);

	int cant_punteros_bloque = obtener_cantidad_punteros(array_bloque_de_punteros);

	log_info(logger, "La cantidad de punteros del bloque es %d", cant_punteros_bloque);

	int pos_nuevo_bloque = (cant_punteros_bloque + 1)*sizeof(uint32_t);

	for(int i = cant_bloques; i > 0; i--){
		usleep(RETARDO_ACCESO_BLOQUE);
		uint32_t bloque_nuevo = buscar_bloque_libre();
		uint32_t* puntero_a_bloque = malloc(sizeof(uint32_t));
		*puntero_a_bloque = bloque_nuevo;

		log_info(logger, "El nuevo bloque asignado es %d", bloque_nuevo);

		usleep(RETARDO_ACCESO_BLOQUE);
		memcpy(array_bloque_de_punteros+pos_nuevo_bloque, puntero_a_bloque, sizeof(uint32_t));
		uint32_t* puntero_leido_testamento = malloc(sizeof(uint32_t));
		memcpy(puntero_leido_testamento, array_bloque_de_punteros+pos_nuevo_bloque, sizeof(uint32_t));
		int log_puntero_leido_testamento = *puntero_leido_testamento;
		log_info(logger, "El puntero escrito en el bloque de punteros es: %d", log_puntero_leido_testamento);
		pos_nuevo_bloque += sizeof(uint32_t);
		cant_punteros_bloque++;
	}

	log_info(logger, "Ahora la cantidad de punteros del bloque es %d", cant_punteros_bloque);

	usleep(RETARDO_ACCESO_BLOQUE);
	memcpy(array_bloque_de_punteros, &cant_punteros_bloque, sizeof(uint32_t));
	usleep(RETARDO_ACCESO_BLOQUE);
	memcpy(buffer_bloques+pos_bloque_punteros, array_bloque_de_punteros, BLOCK_SIZE);
}

void sacar_bloques(int cant_bloques, t_config* archivo){
	int puntero_indirecto = config_get_int_value(archivo, "PUNTERO_INDIRECTO");

	uint32_t* array_bloque_de_punteros = malloc(BLOCK_SIZE);
	int pos_bloque_punteros = puntero_indirecto*BLOCK_SIZE;
	usleep(RETARDO_ACCESO_BLOQUE);
	memcpy(array_bloque_de_punteros, buffer_bloques + pos_bloque_punteros, BLOCK_SIZE);

	usleep(RETARDO_ACCESO_BLOQUE);
	int cant_punteros_bloque = obtener_cantidad_punteros(array_bloque_de_punteros);
	int pos_ultimo_puntero = (cant_punteros_bloque)*sizeof(uint32_t);

	log_info(logger, "La cantidad de punteros del bloque es %d", cant_punteros_bloque);

	int pos_bitmap_ultimo_bloque;

	for(int i = cant_bloques; i > 0; i--){
		usleep(RETARDO_ACCESO_BLOQUE);
		liberar_bloque(pos_ultimo_puntero, array_bloque_de_punteros);

		log_info(logger, "El bloque eliminado es %d", pos_ultimo_puntero);

		usleep(RETARDO_ACCESO_BLOQUE);
		memcpy(&pos_bitmap_ultimo_bloque, array_bloque_de_punteros + pos_ultimo_puntero, sizeof(uint32_t));
		usleep(RETARDO_ACCESO_BLOQUE);

		//log_info(logger_obligatorio, "Acceso Bloque - Archivo: %s - Bloque Archivo: <NUMERO BLOQUE ARCHIVO> - Bloque File System <NUMERO BLOQUE FS>", );

		bitarray_clean_bit(bitmap, pos_bitmap_ultimo_bloque);


		pos_ultimo_puntero -= sizeof(uint32_t);
		cant_punteros_bloque--;
	}

	log_info(logger, "Ahora la cantidad de punteros del bloque es %d", cant_punteros_bloque);

	usleep(RETARDO_ACCESO_BLOQUE);
	memcpy(array_bloque_de_punteros, &cant_punteros_bloque, sizeof(uint32_t));
	usleep(RETARDO_ACCESO_BLOQUE);
	memcpy(buffer_bloques+pos_bloque_punteros, array_bloque_de_punteros, BLOCK_SIZE);
}

void manejar_f_truncate(char* nombre_archivo, int tamanio_nuevo){
	log_info(logger_obligatorio, "Truncar Archivo: %s - Tamaño: %d", nombre_archivo, tamanio_nuevo);
	t_config* archivo_fcb = obtener_archivo(nombre_archivo);
	int tamanio_viejo = config_get_int_value(archivo_fcb, "TAMANIO_ARCHIVO");
	char* texto_tamanio_archivo = malloc(10);
	sprintf(texto_tamanio_archivo, "%d", tamanio_nuevo);

	int tamanio_restante = tamanio_nuevo;
	if(tamanio_viejo == 0){
		asignar_punteros_primer_truncate(archivo_fcb);
		tamanio_restante = tamanio_nuevo - BLOCK_SIZE;
	}

	log_info(logger, "El tamanio nuevo es %s", texto_tamanio_archivo);

	config_set_value(archivo_fcb, "TAMANIO_ARCHIVO", texto_tamanio_archivo);
	config_save(archivo_fcb);

	log_info(logger, "El tamanio viejo era %d", tamanio_viejo);

	if(tamanio_restante > tamanio_viejo){
		// AMPLIAR
		div_t cuenta_bloques_agregar = (tamanio_restante - tamanio_viejo)/BLOCK_SIZE;
		int cantidad_bloques_a_agregar = cuenta_bloques_agregar.quot;
		if(cuenta_bloques_agregar.rem != 0){
			cantidad_bloques_a_agregar++;
		}
		log_info(logger, "La cantidad de bloques a agregar en %s es %d", nombre_archivo, cantidad_bloques_a_agregar);
		asignar_bloques(cantidad_bloques_a_agregar, archivo_fcb);
	} else{
		// REDUCIR
		div_t cuenta_bloques_sacar = (tamanio_viejo - tamanio_restante)/BLOCK_SIZE;
		int cantidad_bloques_a_sacar = cuenta_bloques_sacar.quot;
		if(cuenta_bloques_sacar.rem != 0){
			cantidad_bloques_a_sacar++;
		}
		log_info(logger, "La cantidad de bloques a sacar en %s es %d", nombre_archivo, cantidad_bloques_a_sacar);
		sacar_bloques(cantidad_bloques_a_sacar, archivo_fcb);
	}
}

void asignar_punteros_primer_truncate(t_config* archivo_fcb){

	usleep(RETARDO_ACCESO_BLOQUE);
	int puntero_directo = buscar_bloque_libre();
	usleep(RETARDO_ACCESO_BLOQUE);
	int puntero_indirecto = buscar_bloque_libre();

	char* text_puntero_directo = malloc(10);
	char* text_puntero_indirecto = malloc(10);

	sprintf(text_puntero_directo, "%d", puntero_directo);
	sprintf(text_puntero_indirecto, "%d", puntero_indirecto);

	config_set_value(archivo_fcb, "PUNTERO_DIRECTO", text_puntero_directo);
	config_set_value(archivo_fcb, "PUNTERO_INDIRECTO", text_puntero_indirecto);
	config_save(archivo_fcb);

}

int buscar_bloque(t_config* archivo_fcb, int bloque_a_buscar){

	log_info(logger, "El bloque a buscar pasado por parametro es: %d", bloque_a_buscar);
	if(bloque_a_buscar == 0){
		int puntero_directo = config_get_int_value(archivo_fcb, "PUNTERO_DIRECTO");
		log_info(logger, "El puntero directo es: %d", puntero_directo);
		return puntero_directo*BLOCK_SIZE;
	}
	else{
		int puntero_indirecto = config_get_int_value(archivo_fcb, "PUNTERO_INDIRECTO");

		uint32_t* array_bloque_de_punteros = malloc(BLOCK_SIZE);
		int pos_bloque_punteros = puntero_indirecto*BLOCK_SIZE;
		log_info(logger, "La posicion del bloque de punteros es: %d", pos_bloque_punteros);

		usleep(RETARDO_ACCESO_BLOQUE);
		memcpy(array_bloque_de_punteros, buffer_bloques + pos_bloque_punteros, BLOCK_SIZE);

		uint32_t* puntero_a_bloque_a_buscar = malloc(sizeof(uint32_t));

		int posicion_puntero_bloque_a_buscar = (bloque_a_buscar)*sizeof(uint32_t);
		log_info(logger, "La posicion del puntero del bloque a buscar es: %d", posicion_puntero_bloque_a_buscar);

		memcpy(puntero_a_bloque_a_buscar, array_bloque_de_punteros+posicion_puntero_bloque_a_buscar, sizeof(uint32_t));

		int log_puntero_a_bloque_a_buscar = *puntero_a_bloque_a_buscar;
		log_info(logger, "El puntero a bloque a buscar ahora es: %d", log_puntero_a_bloque_a_buscar);
		return (*puntero_a_bloque_a_buscar)*BLOCK_SIZE;
	}
	log_info(logger, "No debería llegar acá pero llegué. . .");
}

char* leer_datos(t_config* archivo_fcb, int posicion_a_leer, int tamanio){
	div_t cuenta_comienzo_lectura = div(posicion_a_leer, BLOCK_SIZE);

	int bloque_a_leer = cuenta_comienzo_lectura.quot;
	int offset_bloque = cuenta_comienzo_lectura.rem;

	int restante_bloque_comienzo = BLOCK_SIZE - offset_bloque;
	int excedente_lectura = tamanio - restante_bloque_comienzo;

	int tamanio_lectura_primer_bloque = restante_bloque_comienzo;
	if(tamanio < restante_bloque_comienzo){
		tamanio_lectura_primer_bloque = tamanio;
	}

	log_info(logger, "el primer bloque a buscar es %d y el offset en el mismo es %d", bloque_a_leer, offset_bloque);

	char* datos_leidos = malloc(tamanio);

	int posicion_array_bloques_bloque_a_buscar = buscar_bloque(archivo_fcb, bloque_a_leer);
	log_info(logger, "la posicion del bloque en el fs es: %d", posicion_array_bloques_bloque_a_buscar);

	memcpy(datos_leidos, buffer_bloques+posicion_array_bloques_bloque_a_buscar+offset_bloque, tamanio_lectura_primer_bloque);

	if(excedente_lectura > 0){
		log_info(logger, "el excedente lectura es %d", excedente_lectura);
		div_t bloques_a_leer = div(excedente_lectura, BLOCK_SIZE);
		int bloques_a_leer_completos = bloques_a_leer.quot;
		int offset_ultimo_bloque = bloques_a_leer.rem;

		log_info(logger, "hay que leer %d bloques completos", bloques_a_leer_completos);
		log_info(logger, "el offset del ultimo bloque es %d", offset_ultimo_bloque);

		int bloques_extra;
		int desplazamiento_datos_leidos = restante_bloque_comienzo;
		for(bloques_extra = 1; bloques_extra < bloques_a_leer_completos + 1; bloques_extra++){
			int pos_bloque_actual = buscar_bloque(archivo_fcb, bloque_a_leer + bloques_extra);
			memcpy(datos_leidos + desplazamiento_datos_leidos, buffer_bloques + pos_bloque_actual, BLOCK_SIZE);
			desplazamiento_datos_leidos += BLOCK_SIZE;
		}
		int pos_ultimo_bloque = buscar_bloque(archivo_fcb, bloque_a_leer + bloques_extra);
		memcpy(datos_leidos + desplazamiento_datos_leidos, buffer_bloques + pos_ultimo_bloque, offset_ultimo_bloque);
	}

	return datos_leidos;
}

void escribir_datos(t_config* archivo_fcb, int posicion_a_escribir, char* datos_a_escribir, int tamanio_a_escribir){
//	int bloque_a_buscar = floor(posicion_a_escribir/BLOCK_SIZE);

	div_t cuenta_comienzo_escritura = div(posicion_a_escribir, BLOCK_SIZE);
	int bloque_a_escribir = cuenta_comienzo_escritura.quot;
	int offset_bloque = cuenta_comienzo_escritura.rem;

	int restante_bloque_comienzo = BLOCK_SIZE - offset_bloque;
	int excedente_escritura = tamanio_a_escribir - restante_bloque_comienzo;

	int tamanio_escritura_primer_bloque = restante_bloque_comienzo;
	if(tamanio_a_escribir < restante_bloque_comienzo){
		tamanio_escritura_primer_bloque = tamanio_a_escribir;
	}

	log_info(logger, "el bloque a buscar es %d y el offset en el mismo es %d", bloque_a_escribir, offset_bloque);

	int posicion_array_bloques_bloque_a_buscar = buscar_bloque(archivo_fcb, bloque_a_escribir);
	log_info(logger, "la posicion del bloque en el fs es: %d", posicion_array_bloques_bloque_a_buscar);

	memcpy(buffer_bloques+posicion_array_bloques_bloque_a_buscar + offset_bloque, datos_a_escribir, tamanio_escritura_primer_bloque);

	if(excedente_escritura > 0){
		log_info(logger, "el excedente escritura es %d", excedente_escritura);
		div_t bloques_a_escribir = div(excedente_escritura, BLOCK_SIZE);
		int bloques_a_escribir_completos = bloques_a_escribir.quot;
		int offset_ultimo_bloque = bloques_a_escribir.rem;

		log_info(logger, "hay que escribir %d bloques completos", bloques_a_escribir_completos);
		log_info(logger, "el offset del ultimo bloque es %d", offset_ultimo_bloque);

		int bloques_extra;
		int desplazamiento_datos_a_escribir = restante_bloque_comienzo;
		for(bloques_extra = 1; bloques_extra < bloques_a_escribir_completos + 1; bloques_extra++){
			int pos_bloque_actual = buscar_bloque(archivo_fcb, bloque_a_escribir + bloques_extra);
			memcpy(buffer_bloques+pos_bloque_actual, datos_a_escribir + desplazamiento_datos_a_escribir, BLOCK_SIZE);
			desplazamiento_datos_a_escribir += BLOCK_SIZE;
		}
		int pos_ultimo_bloque = buscar_bloque(archivo_fcb, bloque_a_escribir + bloques_extra);
		//log_info(logger, "El bloque a escribir es: %d, Los bloques extra son: %d", bloque_a_escribir, bloques_extra);
		log_info(logger, "La posicion del ultimo bloque despues de usar buscar_bloque es: %d", pos_ultimo_bloque);
		memcpy(buffer_bloques+pos_ultimo_bloque, datos_a_escribir + desplazamiento_datos_a_escribir, offset_ultimo_bloque);
		log_info(logger, "Memcpy de escribir: buffer_bloques + %d, %s + %d, %d", pos_ultimo_bloque, datos_a_escribir, desplazamiento_datos_a_escribir, offset_ultimo_bloque);
	}
}

void manejar_f_read(char* nombre_archivo, int dir_fisica, int tamanio, int posicion_a_leer, int pid){
	//Leer la información correspondiente de los bloques a partir del puntero y el tamaño recibidos
	log_info(logger_obligatorio, "Leer Archivo: %s - Puntero: %d - Memoria: %d - Tamaño: %d", nombre_archivo, posicion_a_leer, dir_fisica, tamanio);
	t_config* archivo_fcb = obtener_archivo(nombre_archivo);
	char* datos_leidos = leer_datos(archivo_fcb, posicion_a_leer, tamanio);
	log_valor_fs(datos_leidos, tamanio);

	//Esta información se deberá enviar a la Memoria para ser escrita a partir de la dirección física recibida por parámetro
	send_escribir_valor_fs(datos_leidos, dir_fisica, tamanio, pid, fd_memoria);
	recv_fin_escritura(fd_memoria);
}

void manejar_f_write(char* nombre_archivo, int dir_fisica, int tamanio, int posicion_a_escribir, int pid){
	log_info(logger_obligatorio, "Escribir Archivo: %s - Puntero: %d - Memoria: %d - Tamaño: %d", nombre_archivo, posicion_a_escribir, dir_fisica, tamanio);
	t_config* archivo_fcb = obtener_archivo(nombre_archivo);

	//Solicitar a Memoria la información que se encuentra a partir de la dirección física
	send_leer_valor_fs(dir_fisica, tamanio, pid,fd_memoria);			//Creo que con esta funcion solicito la info
	char* datos_a_escribir = recv_valor_leido_fs(fd_memoria);

	//Escribir los datos en los bloques correspondientes del archivo a partir del puntero recibido.
	escribir_datos(archivo_fcb, posicion_a_escribir, datos_a_escribir, tamanio);
	log_valor_fs(datos_a_escribir, tamanio);
}

void log_valor_fs(char* valor, int tamanio){
	char* valor_log = malloc(tamanio);
	memcpy(valor_log, valor, tamanio);
	memcpy(valor_log + tamanio, "\0", 1);
	int tamanio_valor = strlen(valor_log);
	log_info(logger, "se leyo/escribio %s de tamaño %d en el espacio de usuario", valor_log, tamanio_valor);
}

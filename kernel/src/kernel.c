#include "../include/kernel.h"

int main(int argc, char **argv) {
	if (argc > 2) {
		return EXIT_FAILURE;
	}
	config = config_create(argv[1]);
	if (config == NULL) {
		log_error(logger, "No se encontró el archivo :(");
		config_destroy(config);
		exit(1);
	}
	leer_config();
	logger = log_create("kernel.log", "kernel_main", 1, LOG_LEVEL_INFO);
	// TODO: cambiar archivo de logs obligatorios
	logger_obligatorio = log_create("kernel.log", "kernel_obligatorio", 1, LOG_LEVEL_INFO);
	inicializar_variables();
	// Conecto kernel con cpu, memoria y filesystem
	fd_cpu = -1, fd_memoria = -1, fd_filesystem = -1;
	if (!generar_conexiones()) {
		log_error(logger, "Alguna conexion falló :(");
		terminar_programa();
		exit(1);
	}

	// Intercambio de mensajes...

	enviar_mensaje("Hola, Soy Kernel!", fd_filesystem);
	enviar_mensaje("Hola, Soy Kernel!", fd_memoria);
	enviar_mensaje("Hola, Soy Kernel!", fd_cpu);

	planificar();

	server_socket = iniciar_servidor(logger, IP, PUERTO);
	while (server_escuchar(server_socket));

	terminar_programa();
	return 0;
}

// ------------------ INIT ------------------

void leer_config() {
	IP = config_get_string_value(config, "IP_ESCUCHA");
	PUERTO = config_get_string_value(config, "PUERTO_ESCUCHA");
	IP_FILESYSTEM = config_get_string_value(config, "IP_FILESYSTEM");
	PUERTO_FILESYSTEM = config_get_string_value(config, "PUERTO_FILESYSTEM");
	IP_CPU = config_get_string_value(config, "IP_CPU");
	PUERTO_CPU = config_get_string_value(config, "PUERTO_CPU");
	IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
	PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");
	char *algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	asignar_algoritmo(algoritmo);
	ESTIMACION_INICIAL = config_get_int_value(config, "ESTIMACION_INICIAL");
	HRRN_ALFA = config_get_double_value(config, "HRRN_ALFA");
	GRADO_MAX_MULTIPROGRAMACION = config_get_int_value(config, "GRADO_MAX_MULTIPROGRAMACION");
	RECURSOS = config_get_array_value(config, "RECURSOS");
	char** instancias = string_array_new();
	instancias = config_get_array_value(config, "INSTANCIAS_RECURSOS");
	INSTANCIAS_RECURSOS = string_to_int_array(instancias);
	string_array_destroy(instancias);		//Sigue sin funcar el free a instancias
}

int* string_to_int_array(char** array_de_strings){
	int count = string_array_size(array_de_strings);
	int *numbers = malloc(sizeof(int) * count);
	for(int i = 0; i < count; i++){
		int num = atoi(array_de_strings[i]);
		numbers[i] = num;
	}
	return numbers;
}

void asignar_algoritmo(char *algoritmo) {
	if (strcmp(algoritmo, "FIFO") == 0) {
		ALGORITMO_PLANIFICACION = FIFO;
	} else if (strcmp(algoritmo, "HRRN") == 0) {
		ALGORITMO_PLANIFICACION = HRRN;
	}else{
		log_error(logger, "El algoritmo no es valido");
	}
}

bool generar_conexiones() {
	pthread_t conexion_filesystem;
	pthread_t conexion_cpu;

	fd_filesystem = crear_conexion(IP_FILESYSTEM, PUERTO_FILESYSTEM);
	pthread_create(&conexion_filesystem, NULL, (void*) procesar_conexion_fs, (void*) &fd_filesystem);
	pthread_detach(conexion_filesystem);

	fd_cpu = crear_conexion(IP_CPU, PUERTO_CPU);
	pthread_create(&conexion_cpu, NULL, (void*) procesar_conexion, (void*) &fd_cpu);
	pthread_detach(conexion_cpu);

	fd_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);

	return fd_filesystem != -1 && fd_cpu != -1 && fd_memoria != -1;
}

void inicializar_variables() {
	//PCBs
	generador_pid = 1;
	lista_ready = list_create();
	cola_exit = list_create();
	cola_listos_para_ready = list_create();
	cola_exec = list_create();
	cola_block = list_create();
	cola_block_io = list_create();
	cola_block_fs = list_create();
	lista_recursos = inicializar_recursos();
	fs_mem_op_count = 0;
	archivos_abiertos = list_create();

	//Semaforos
	pthread_mutex_init(&mutex_generador_pid, NULL);
	pthread_mutex_init(&mutex_cola_ready, NULL);
	pthread_mutex_init(&mutex_cola_listos_para_ready, NULL);
	pthread_mutex_init(&mutex_cola_exit, NULL);
	pthread_mutex_init(&mutex_cola_exec, NULL);
	pthread_mutex_init(&mutex_cola_block, NULL);
	pthread_mutex_init(&mutex_cola_block_io, NULL);
	pthread_mutex_init(&mutex_cola_block_fs, NULL);

	sem_init(&sem_multiprog, 0, GRADO_MAX_MULTIPROGRAMACION);
	sem_init(&sem_listos_ready, 0, 0);
	sem_init(&sem_ready, 0, 0);
	sem_init(&sem_exec, 0, 1);
	sem_init(&sem_exit, 0, 0);
	sem_init(&sem_block_return, 0, 0);
	sem_init(&ongoing_fs_mem_op, 0, 1);
	sem_init(&fin_f_open, 0, 0);

}

t_list* inicializar_recursos(){
	t_list* lista = list_create();
	int cantidad_recursos = string_array_size(RECURSOS);
	for(int i = 0; i < cantidad_recursos; i++){
		char* string = RECURSOS[i];
		t_recurso* recurso = malloc(sizeof(t_recurso));
		recurso->recurso = malloc(sizeof(char) * strlen(string) + 1);
		strcpy(recurso->recurso, string);
		t_list* cola_block = list_create();
		recurso->id = i;
		recurso->instancias = INSTANCIAS_RECURSOS[i];
		recurso->cola_block_asignada = cola_block;
		pthread_mutex_init(&recurso->mutex_asignado, NULL);
		list_add(lista, recurso);
	}
	return lista;
}

void inicializar_registro(t_contexto_ejecucion* contexto){
	contexto->registros = malloc(sizeof(t_registros));

	contexto->registros->ax = malloc(sizeof(5));
	contexto->registros->bx = malloc(sizeof(5));
	contexto->registros->cx = malloc(sizeof(5));
	contexto->registros->dx = malloc(sizeof(5));
	contexto->registros->eax = malloc(sizeof(9));
	contexto->registros->ebx = malloc(sizeof(9));
	contexto->registros->ecx = malloc(sizeof(9));
	contexto->registros->edx = malloc(sizeof(9));
	contexto->registros->rax = malloc(sizeof(17));
	contexto->registros->rbx = malloc(sizeof(17));
	contexto->registros->rcx = malloc(sizeof(17));
	contexto->registros->rdx = malloc(sizeof(17));

	contexto->registros->ax = "";
	contexto->registros->bx = "";
	contexto->registros->cx = "";
	contexto->registros->dx = "";
	contexto->registros->eax = "";
	contexto->registros->ebx = "";
	contexto->registros->ecx = "";
	contexto->registros->edx = "";
	contexto->registros->rax = "";
	contexto->registros->rbx = "";
	contexto->registros->rcx = "";
	contexto->registros->rdx = "";
}

void terminar_programa(){
	log_destroy(logger);
	log_destroy(logger_obligatorio);
	config_destroy(config);
	free(INSTANCIAS_RECURSOS);
	free(RECURSOS);
}

// ------------------ COMUNICACION ------------------

void procesar_conexion(void* void_args) {
	int *args = (int*) void_args;
	int cliente_socket = *args;

	op_code cop;

	while (cliente_socket != -1) {
		cop = recibir_operacion(cliente_socket);
		if (cop == -1) {
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
		case INSTRUCCIONES_CONSOLA:
			t_list *instrucciones = recv_instrucciones(logger, cliente_socket);
			armar_pcb(instrucciones, cliente_socket);
			break;
		case CONTEXTO_EJECUCION:
			t_contexto_ejecucion* contexto_recibido = recv_contexto_ejecucion(cliente_socket);
			log_info(logger, "recibi un contexto de ejecucion");
			t_pcb* pcb = safe_pcb_remove(cola_exec, &mutex_cola_exec);
			actualizar_contexto_pcb(pcb, contexto_recibido);
			recv(cliente_socket, &cop, sizeof(op_code), 0);
			switch(cop){
			case CAMBIAR_ESTADO:
				log_info(logger, "recibi un aviso de cambio de estado");
				estado_proceso nuevo_estado = recv_cambiar_estado(cliente_socket);
				calcular_estimacion(pcb);
				procesar_cambio_estado(pcb, nuevo_estado);
				sem_post(&sem_exec);
				break;
			case MANEJAR_IO:
				int tiempo = recv_tiempo_io(cliente_socket);
				calcular_estimacion(pcb);
				cambiar_estado(pcb, BLOCK);
				sem_post(&sem_exec);
				manejar_io(pcb, tiempo);
				break;
			case MANEJAR_WAIT:
				char* recurso_wait = recv_recurso(cliente_socket);
				manejar_wait(pcb, recurso_wait);
				free(recurso_wait);
				break;
			case MANEJAR_SIGNAL:
				char* recurso_signal = recv_recurso(cliente_socket);
				manejar_signal(pcb, recurso_signal);
				free(recurso_signal);
				break;
			case MANEJAR_CREATE_SEGMENT:
				t_list* create_sgm_params = recv_create_segment(cliente_socket);
				int* id_segmento = list_get(create_sgm_params, 0);
				int* tamanio = list_get(create_sgm_params, 1);
				log_info(logger, "manejo create_segment");
				manejar_create_segment(pcb, cliente_socket, *id_segmento, *tamanio);
				list_destroy(create_sgm_params);
				break;
			case MANEJAR_DELETE_SEGMENT:
				t_list* delete_sgm_params = recv_delete_segment(cliente_socket);
				int* id = list_get(delete_sgm_params, 0);
				log_info(logger, "manejo delete_segment");
				send_delete_segment(pcb->contexto_de_ejecucion->pid, *id, fd_memoria);
				t_list* tabla_segmentos_actualizada = recv_tabla_segmentos(fd_memoria);
				log_info(logger, "recibi la ts actualizada de memoria de tamaño %d", list_size(tabla_segmentos_actualizada));
				pcb->contexto_de_ejecucion->tabla_de_segmentos = tabla_segmentos_actualizada;
				safe_pcb_add(cola_exec, pcb, &mutex_cola_exec);
				send_contexto_ejecucion(pcb->contexto_de_ejecucion,cliente_socket);
				break;
			case MANEJAR_F_READ:
				t_list* f_read_params = recv_manejo_f_read(cliente_socket);
				char* nombre_archivo_read = list_get(f_read_params, 0);
				int* dir_fisica_read = list_get(f_read_params, 1);
				int* cant_bytes_read = list_get(f_read_params, 2);

				if(fs_mem_op_count == 0){
					sem_wait(&ongoing_fs_mem_op);
				}
				fs_mem_op_count++;

				// manejo f_read
				t_archivo* archivo_read = get_archivo_global(nombre_archivo_read);
				log_info(logger_obligatorio, "PID: %d - Leer Archivo: %s - Puntero %d - Dirección Memoria %d - Tamaño %d", pcb->contexto_de_ejecucion->pid, nombre_archivo_read, archivo_read->puntero, *dir_fisica_read, *cant_bytes_read);
				log_info(logger, "bloqueo al proceso %d por f_read,  fs_mem_op_count: %d", pcb->contexto_de_ejecucion->pid, fs_mem_op_count);
				bloquear_pcb_por_fs(pcb);
				log_info(logger_obligatorio, "PID: %d - Bloqueado por: %s", pcb->contexto_de_ejecucion->pid, nombre_archivo_read);

				send_manejar_f_read_fs(nombre_archivo_read, *dir_fisica_read, *cant_bytes_read, archivo_read->puntero, pcb->contexto_de_ejecucion->pid, fd_filesystem);
				sem_post(&sem_exec);
				break;
			case MANEJAR_F_WRITE:
				t_list* f_write_params = recv_manejo_f_write(cliente_socket);
				char* nombre_archivo_write = list_get(f_write_params, 0);
				int* dir_fisica_write = list_get(f_write_params, 1);
				int* cant_bytes_write = list_get(f_write_params, 2);

				if(fs_mem_op_count == 0){
					sem_wait(&ongoing_fs_mem_op);
				}
				fs_mem_op_count++;

				// manejo f_write
				t_archivo* archivo_write = get_archivo_global(nombre_archivo_write);

				log_info(logger_obligatorio, "PID: %d - Escribir Archivo: %s - Puntero %d - Dirección Memoria %d - Tamaño %d", pcb->contexto_de_ejecucion->pid, nombre_archivo_write, archivo_write->puntero, *dir_fisica_write, *cant_bytes_write);
				log_info(logger, "bloqueo al proceso %d por f_write,  fs_mem_op_count: %d", pcb->contexto_de_ejecucion->pid, fs_mem_op_count);
				bloquear_pcb_por_fs(pcb);
				log_info(logger_obligatorio, "PID: %d - Bloqueado por: %s", pcb->contexto_de_ejecucion->pid, nombre_archivo_write);

				send_manejar_f_write_fs(nombre_archivo_write, *dir_fisica_write, *cant_bytes_write, archivo_write->puntero, pcb->contexto_de_ejecucion->pid, fd_filesystem);
				sem_post(&sem_exec);
				break;
			case MANEJAR_F_OPEN:
				char* nombre_archivo_open = recv_manejo_f_open(cliente_socket);
				log_info(logger_obligatorio, "PID: %d - Abrir Archivo: %s", pcb->contexto_de_ejecucion->pid, nombre_archivo_open);
				if(!archivo_is_opened(nombre_archivo_open)){
					ejecutar_f_open(nombre_archivo_open, pcb);
					safe_pcb_add(cola_exec, pcb, &mutex_cola_exec);
					send_contexto_ejecucion(pcb->contexto_de_ejecucion, cliente_socket);
				}else{
					t_archivo* archivo_open_global = get_archivo_global(nombre_archivo_open);
					log_info(logger, "bloqueo al proceso %d porque el archivo %s ya estaba abierto", pcb->contexto_de_ejecucion->pid, nombre_archivo_open);
					cambiar_estado(pcb, BLOCK);
					calcular_estimacion(pcb);
					log_info(logger_obligatorio, "PID: %d - Bloqueado por: %s", pcb->contexto_de_ejecucion->pid, nombre_archivo_write);

					list_add(pcb->archivos_abiertos, archivo_open_global);
					log_info(logger, "el proceso %d tiene %d archivos abiertos", pcb->contexto_de_ejecucion->pid, list_size(pcb->archivos_abiertos));

					safe_pcb_add(archivo_open_global->cola_block_asignada, pcb, &(archivo_open_global->mutex_asignado));
					pthread_mutex_lock(&(archivo_open_global->mutex_asignado));
					int cant_proc_archivo_open = list_size(archivo_open_global->cola_block_asignada);
					pthread_mutex_unlock(&(archivo_open_global->mutex_asignado));
					log_info(logger, "el archivo %s tiene %d procesos abiertos", archivo_open_global->nombre_archivo, cant_proc_archivo_open);

					sem_post(&sem_exec);
				}
				break;
			case MANEJAR_F_CLOSE:
				char* nombre_archivo_close = recv_manejo_f_close(cliente_socket);
				log_info(logger_obligatorio, "PID: %d - Cerrar Archivo: %s", pcb->contexto_de_ejecucion->pid, nombre_archivo_close);
				t_archivo* archivo_close = quitar_archivo_de_tabla_proceso(nombre_archivo_close, pcb);
				pthread_mutex_lock(&(archivo_close->mutex_asignado));
				if(!list_is_empty(archivo_close->cola_block_asignada)){
					t_pcb* pcb_bloqueado = safe_pcb_remove(archivo_close->cola_block_asignada, &(archivo_close->mutex_asignado));
					pthread_mutex_lock(&(archivo_close->mutex_asignado));
					int cant_proc_archivo_close = list_size(archivo_close->cola_block_asignada);
					pthread_mutex_unlock(&(archivo_close->mutex_asignado));
					log_info(logger, "el archivo %s tiene %d procesos abiertos", archivo_close->nombre_archivo, cant_proc_archivo_close);
					log_info(logger, "Desbloqueo al proceso %d bloqueado por archivo %s", pcb_bloqueado->contexto_de_ejecucion->pid, nombre_archivo_close);
					safe_pcb_add(cola_block, pcb_bloqueado, &mutex_cola_block);
					sem_post(&sem_block_return);
				}else{
					list_remove_element(archivos_abiertos, archivo_close);
					log_info(logger, "Cierro el archivo %s, quedan %d archivos abiertos", nombre_archivo_close, list_size(archivos_abiertos));
				}
				pthread_mutex_unlock(&(archivo_close->mutex_asignado));
				safe_pcb_add(cola_exec, pcb, &mutex_cola_exec);
				send_contexto_ejecucion(pcb->contexto_de_ejecucion,cliente_socket);
				break;
			case MANEJAR_F_SEEK:
				t_list* f_seek_params = recv_manejo_f_seek(cliente_socket);
				char* nombre_archivo_seek = list_get(f_seek_params, 0);
				int* puntero_seek = list_get(f_seek_params, 1);
				log_info(logger_obligatorio, "PID: %d - Actualizar puntero Archivo: %s - Puntero %d", pcb->contexto_de_ejecucion->pid, nombre_archivo_seek, *puntero_seek);
				t_archivo* archivo_seek = get_archivo_global(nombre_archivo_seek);
				archivo_seek->puntero = *puntero_seek;
				safe_pcb_add(cola_exec, pcb, &mutex_cola_exec);
				send_contexto_ejecucion(pcb->contexto_de_ejecucion,cliente_socket);
				break;
			case MANEJAR_F_TRUNCATE:
				t_list* f_truncate_params = recv_manejo_f_truncate(cliente_socket);
				char* nombre_archivo_truncate = list_get(f_truncate_params, 0);
				int* tamanio_truncate = list_get(f_truncate_params, 1);
				log_info(logger_obligatorio, "PID: %d - Truncar Archivo: %s - Tamaño: %d", pcb->contexto_de_ejecucion->pid, nombre_archivo_truncate, *tamanio_truncate);
				bloquear_pcb_por_fs(pcb);
				log_info(logger_obligatorio, "PID: %d - Bloqueado por: %s", pcb->contexto_de_ejecucion->pid, nombre_archivo_truncate);
				send_manejar_f_truncate(nombre_archivo_truncate, *tamanio_truncate, fd_filesystem);
				sem_post(&sem_exec);
				break;
			default:
				log_error(logger, "Codigo de operacion no reconocido en el server de %s", server_name);
				return;
			}
			break;
		default:
			log_error(logger, "Codigo de operacion no reconocido en el server de %s", server_name);
			log_info(logger, "el numero del cop es: %d", cop);
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
	server_name = "Kernel";
	int cliente_socket = esperar_cliente(logger, server_name, server_socket);

	if (cliente_socket != -1) {
		pthread_t hilo;
		pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) &cliente_socket);
		pthread_detach(hilo);
		return 1;
	}
	return 0;
}

void procesar_conexion_fs(void* void_args) {
	int* args = (int*) void_args;
	int cliente_socket = *args;

	op_code cop;
	while (cliente_socket != -1) {
		cop = recibir_operacion(cliente_socket);
		if (cop == -1) {
			log_info(logger, "El cliente se desconecto de %s server", server_name);
			return;
		}
		t_pcb* pcb_block_fs = safe_pcb_remove(cola_block_fs, &mutex_cola_block_fs);
		pthread_mutex_lock(&mutex_cola_block_fs);
		int cant_pcbs_block_fs = list_size(cola_block_fs);
		pthread_mutex_unlock(&mutex_cola_block_fs);
		log_info(logger, "saque el proceso %d de la cola de bloqueados del fs, quedan %d", pcb_block_fs->contexto_de_ejecucion->pid, cant_pcbs_block_fs);
		switch(cop){
		case FIN_F_OPEN:
			recv_fin_f_open(cliente_socket);
			log_info(logger, "el fs termino de abrir un archivo del proceso %d", pcb_block_fs->contexto_de_ejecucion->pid);
			sem_post(&fin_f_open);
			break;
		case FIN_F_READ:
			recv_fin_f_read(cliente_socket);
			fs_mem_op_count--;
			if(fs_mem_op_count == 0){
				sem_post(&ongoing_fs_mem_op);
			}
			log_info(logger, "el fs termino de leer un archivo del proceso %d, fs_mem_op_count: %d", pcb_block_fs->contexto_de_ejecucion->pid, fs_mem_op_count);
			// manejo fin f_read...
			safe_pcb_add(cola_block, pcb_block_fs, &mutex_cola_block_fs);
			sem_post(&sem_block_return);
			break;
		case FIN_F_WRITE:
			recv_fin_f_write(cliente_socket);
			fs_mem_op_count--;
			if(fs_mem_op_count == 0){
				sem_post(&ongoing_fs_mem_op);
			}
			log_info(logger, "el fs termino de escribir un archivo del proceso %d, fs_mem_op_count: %d", pcb_block_fs->contexto_de_ejecucion->pid, fs_mem_op_count);

			// manejo fin f_write...
			safe_pcb_add(cola_block, pcb_block_fs, &mutex_cola_block_fs);
			sem_post(&sem_block_return);
			break;
		case FIN_F_TRUNCATE:
			recv_fin_f_truncate(cliente_socket);
			log_info(logger, "el fs termino de truncar un archivo del proceso %d", pcb_block_fs->contexto_de_ejecucion->pid);
			safe_pcb_add(cola_block, pcb_block_fs, &mutex_cola_block_fs);
			sem_post(&sem_block_return);
			break;
		default:
			log_error(logger, "Codigo de operacion no reconocido en la comunicacion con fs");
			log_info(logger, "el numero del cop es: %d", cop);
			return;
		}
	}
}
// ------------------ PCBS ------------------

t_pcb* pcb_create(t_list* instrucciones, int pid, int cliente_socket) {
	t_pcb *pcb = malloc(sizeof(t_pcb));
	pcb->fd_consola = cliente_socket;
	pcb->estimado_proxima_rafaga = ESTIMACION_INICIAL;
	pcb->archivos_abiertos = list_create();

	t_contexto_ejecucion* contexto = malloc(sizeof(t_contexto_ejecucion));
	pcb->contexto_de_ejecucion = contexto;
	pcb->contexto_de_ejecucion->pid = pid;
	pcb->contexto_de_ejecucion->program_counter = 0;
	pcb->contexto_de_ejecucion->instrucciones = instrucciones;
	inicializar_registro(contexto);

	send_inicializar_proceso(pid, fd_memoria);
	t_list* tabla_segmentos = recv_proceso_inicializado(fd_memoria);
	pcb->contexto_de_ejecucion->tabla_de_segmentos = tabla_segmentos;
	pcb->contexto_de_ejecucion->estado = NEW;

	return pcb;
}

// hay que acordarse de agregar frees aca si se cambia la estructura del t_pcb !!!
void pcb_destroy(t_pcb* pcb){
	contexto_destroyer(pcb->contexto_de_ejecucion);
	free(pcb);
}

void cambiar_estado(t_pcb *pcb, estado_proceso nuevo_estado) {
	if(pcb->contexto_de_ejecucion->estado != nuevo_estado){
		char *nuevo_estado_string = strdup(estado_to_string(nuevo_estado));
		char *estado_anterior_string = strdup(estado_to_string(pcb->contexto_de_ejecucion->estado));
		pcb->contexto_de_ejecucion->estado = nuevo_estado;
		log_info(logger_obligatorio, "PID: %d - Estado Anterior: %s - Estado Actual: %s", pcb->contexto_de_ejecucion->pid, estado_anterior_string, nuevo_estado_string);
		free(estado_anterior_string);
		free(nuevo_estado_string);
	}
}

void procesar_cambio_estado(t_pcb* pcb, estado_proceso estado_nuevo){

	switch(estado_nuevo){
	case READY:
		set_pcb_ready(pcb);
		sem_post(&sem_ready);
		break;
	case FINISH_EXIT:
		cambiar_estado(pcb, estado_nuevo);
		pcb->contexto_de_ejecucion->motivo_exit = SUCCESS;
		safe_pcb_add(cola_exit, pcb, &mutex_cola_exit);
		sem_post(&sem_exit);
		break;
	case FINISH_ERROR:
		cambiar_estado(pcb, estado_nuevo);
		safe_pcb_add(cola_exit, pcb, &mutex_cola_exit);
		sem_post(&sem_exit);
		break;
	default:
		log_error(logger, "Cambio de estado no reconocido");
		break;
	}
}

void armar_pcb(t_list *instrucciones, int cliente_socket) {
	pthread_mutex_lock(&mutex_generador_pid);
	int pid = generador_pid;
	generador_pid++;
	pthread_mutex_unlock(&mutex_generador_pid);
	t_pcb *pcb = pcb_create(instrucciones, pid, cliente_socket);
	log_info(logger_obligatorio, "Se crea el proceso %d en NEW", pid);
	safe_pcb_add(cola_listos_para_ready, pcb, &mutex_cola_listos_para_ready);
	sem_post(&sem_listos_ready);
}

void actualizar_contexto_pcb(t_pcb* pcb, t_contexto_ejecucion* contexto){
	//TODO Revisar con valgrind si hay que hacer un contexto_destroyer
	pcb->contexto_de_ejecucion = contexto;
}

void actualizar_registros(t_pcb* pcb, t_contexto_ejecucion* contexto){
	strcpy(pcb->contexto_de_ejecucion->registros->ax, contexto->registros->ax);
	strcpy(pcb->contexto_de_ejecucion->registros->bx, contexto->registros->bx);
	strcpy(pcb->contexto_de_ejecucion->registros->cx, contexto->registros->cx);
	strcpy(pcb->contexto_de_ejecucion->registros->dx, contexto->registros->dx);
	strcpy(pcb->contexto_de_ejecucion->registros->eax, contexto->registros->eax);
	strcpy(pcb->contexto_de_ejecucion->registros->ebx, contexto->registros->ebx);
	strcpy(pcb->contexto_de_ejecucion->registros->ecx, contexto->registros->ecx);
	strcpy(pcb->contexto_de_ejecucion->registros->edx, contexto->registros->edx);
	strcpy(pcb->contexto_de_ejecucion->registros->rax, contexto->registros->rax);
	strcpy(pcb->contexto_de_ejecucion->registros->rbx, contexto->registros->rbx);
	strcpy(pcb->contexto_de_ejecucion->registros->rcx, contexto->registros->rcx);
	strcpy(pcb->contexto_de_ejecucion->registros->rdx, contexto->registros->rdx);
}

void actualizar_ts_de_pcbs(t_list* lista_ts_wrappers){
	log_info(logger, "Comienzo actualizacion de ts de pcbs");
	actualizar_ts_de_pcbs_de_cola(lista_ts_wrappers, lista_ready, &mutex_cola_ready);
	actualizar_ts_de_pcbs_de_cola(lista_ts_wrappers, cola_listos_para_ready, &mutex_cola_listos_para_ready);
	actualizar_ts_de_pcbs_de_cola(lista_ts_wrappers, cola_exec, &mutex_cola_exec);
	actualizar_ts_de_pcbs_de_cola(lista_ts_wrappers, cola_block_io, &mutex_cola_block_io);
	actualizar_ts_de_pcbs_de_cola(lista_ts_wrappers, cola_block, &mutex_cola_block);
	actualizar_ts_de_pcbs_de_cola(lista_ts_wrappers, cola_exit, &mutex_cola_exit);

	// TODO falta testear esto
	for(int i = 0; i < list_size(lista_recursos); i++){
		t_recurso* recurso = list_get(lista_recursos, i);
		actualizar_ts_de_pcbs_de_cola(lista_ts_wrappers, recurso->cola_block_asignada, &(recurso->mutex_asignado));
	}

	for(int i = 0; i < list_size(archivos_abiertos); i++){
		t_archivo* archivo = list_get(archivos_abiertos, i);
		actualizar_ts_de_pcbs_de_cola(lista_ts_wrappers, archivo->cola_block_asignada, &(archivo->mutex_asignado));
	}

}

void actualizar_ts_de_pcbs_de_cola(t_list* lista_ts_wrappers, t_list* lista_pcb, pthread_mutex_t* mutex_cola){
	pthread_mutex_lock(mutex_cola);
	for (int i = 0; i < list_size(lista_pcb); i++){
		t_pcb* pcb = list_get(lista_pcb, i);
		t_list* ts_actualizada = get_ts_from_pid(pcb->contexto_de_ejecucion->pid, lista_ts_wrappers);
		if(ts_actualizada != NULL) {
			// falta free de la tabla_de_segmentos vieja pero no se donde ponerlo para q no rompa ;.;
			pcb->contexto_de_ejecucion->tabla_de_segmentos = ts_actualizada;
			log_ts_de_pid(logger, pcb->contexto_de_ejecucion->pid, pcb->contexto_de_ejecucion->tabla_de_segmentos);
		}
	}
	pthread_mutex_unlock(mutex_cola);
}

t_list* get_ts_from_pid(int pid, t_list* lista_ts_wrappers){
	for (int i = 0; i < list_size(lista_ts_wrappers); i++){
		ts_wrapper* wrapper = list_get(lista_ts_wrappers, i);
		if(wrapper->pid == pid){
			return wrapper->tabla_de_segmentos;
		}
	}
	log_info(logger, "no encontre el pid %d en la lista_ts_wrappers", pid);
	return NULL;
}
// ------------------ PLANIFICACION ------------------
// Por ahora hago esto aca, cuando funque vemos si lo ponemos en un lugar aparte.

void planificar() {
	planificar_largo_plazo();
	log_info(logger, "Se inició la planificacion de largo plazo");
	planificar_corto_plazo();
	log_info(logger, "Se inició la planificacion de corto plazo");
}

void planificar_largo_plazo() {
	pthread_t hilo_ready;
	pthread_t hilo_exit;
	pthread_t hilo_block;
	pthread_create(&hilo_exit, NULL, (void*) exit_pcb, NULL);
	pthread_create(&hilo_ready, NULL, (void*) ready_pcb, NULL);
	pthread_create(&hilo_block, NULL, (void*)block_return_pcb, NULL);
	pthread_detach(hilo_exit);
	pthread_detach(hilo_ready);
	pthread_detach(hilo_block);
}

void exit_pcb(void) {
	while (1)
	{
		sem_wait(&sem_exit);
		t_pcb *pcb = safe_pcb_remove(cola_exit, &mutex_cola_exit);
		char* motivo = motivo_exit_to_string(pcb->contexto_de_ejecucion->motivo_exit);
		log_info(logger_obligatorio, "Finaliza el proceso %d - Motivo: %s", pcb->contexto_de_ejecucion->pid, motivo);
		enviar_mensaje("Fin del proceso", pcb->fd_consola);
		send_terminar_proceso(pcb->contexto_de_ejecucion->pid, fd_memoria);
		pcb_destroy(pcb);
		sem_post(&sem_multiprog);
	}
}

void ready_pcb(void) {
	while (1) {
		sem_wait(&sem_listos_ready);
		t_pcb *pcb = safe_pcb_remove(cola_listos_para_ready, &mutex_cola_listos_para_ready);
		sem_wait(&sem_multiprog);
		set_pcb_ready(pcb);
		sem_post(&sem_ready);
	}
}

void block_return_pcb(){
	while(1){
		sem_wait(&sem_block_return);
		t_pcb* pcb = safe_pcb_remove(cola_block, &mutex_cola_block);
		set_pcb_ready(pcb);
		sem_post(&sem_ready);
	}
}

t_pcb* safe_pcb_remove(t_list* list, pthread_mutex_t* mutex){
	t_pcb* pcb;
	pthread_mutex_lock(mutex);
	pcb = list_remove(list, 0);
	pthread_mutex_unlock(mutex);
	return pcb;
}

void safe_pcb_add(t_list* list, t_pcb* pcb, pthread_mutex_t* mutex){
	pthread_mutex_lock(mutex);
	list_add(list, pcb);
	pthread_mutex_unlock(mutex);
}

void set_pcb_ready(t_pcb *pcb) {
	pthread_mutex_lock(&mutex_cola_ready);
	cambiar_estado(pcb, READY);
	pcb->tiempo_ingreso_ready = time(NULL);
	// no safe_pcb_add
	list_add(lista_ready, pcb);
	log_cola_ready();
	pthread_mutex_unlock(&mutex_cola_ready);
}

void log_cola_ready(){
	t_list *lista_a_loguear = pcb_to_pid_list(lista_ready);
	char *lista = list_to_string(lista_a_loguear);
	log_info(logger_obligatorio, "Cola Ready %s: [%s]", algoritmo_to_string(ALGORITMO_PLANIFICACION), lista);
	list_destroy(lista_a_loguear);
	free(lista);
}

t_list *pcb_to_pid_list(t_list *list)
{
	t_list* lista_de_pids = list_create();
    for (int i = 0; i < list_size(list); i++)
    {
        t_pcb* pcb = list_get(list, i);
        list_add(lista_de_pids, &(pcb->contexto_de_ejecucion->pid));
    }
    return lista_de_pids;
}

char* algoritmo_to_string(t_algoritmo algoritmo){
	switch(algoritmo){
	case FIFO: return "FIFO";
	case HRRN: return "HRRN";
	default: return NULL;
	}
}

void planificar_corto_plazo() {
	pthread_t hilo_corto_plazo;
	pthread_create(&hilo_corto_plazo, NULL, (void*)exec_pcb, NULL);
	pthread_detach(hilo_corto_plazo);
}

void exec_pcb(){
	while (1) {
		sem_wait(&sem_ready);
		sem_wait(&sem_exec);
		t_pcb *pcb = elegir_pcb_segun_algoritmo();
		dispatch(pcb);
	}
}

t_pcb* elegir_pcb_segun_algoritmo(){
	switch (ALGORITMO_PLANIFICACION) {
	case FIFO:
		return safe_pcb_remove(lista_ready, &mutex_cola_ready);
	case HRRN:
		return obtener_pcb_HRRN();
	default:
		log_error(logger, "No se reconocio el algoritmo de planifacion");
		exit(1);
	}
}

void dispatch(t_pcb* pcb){
	cambiar_estado(pcb, EXEC);
	log_info(logger, "El proceso %d se pone en ejecucion", pcb->contexto_de_ejecucion->pid);
	pcb->tiempo_ingreso_exec = time(NULL);
	safe_pcb_add(cola_exec, pcb, &mutex_cola_exec);
	send_contexto_ejecucion(pcb->contexto_de_ejecucion, fd_cpu);
}

t_pcb* obtener_pcb_HRRN(){
	pthread_mutex_lock(&mutex_cola_ready);
	list_sort(lista_ready, (void*)maximo_HRRN);
	t_pcb* pcb = list_remove(lista_ready, 0);
	log_info(logger, "Se eligio el proceso %d por HRRN", pcb->contexto_de_ejecucion->pid);
	pthread_mutex_unlock(&mutex_cola_ready);
	return pcb;
}

bool maximo_HRRN(t_pcb* pcb1, t_pcb* pcb2){
	return response_ratio(pcb1) >= response_ratio(pcb2);
}

double response_ratio(t_pcb* pcb){
	time_t tiempo_actual = time(NULL);
	double espera = difftime(tiempo_actual, pcb->tiempo_ingreso_ready);
	double rr = (pcb->estimado_proxima_rafaga + espera * 1000) / pcb->estimado_proxima_rafaga;
	log_info(logger, "El response ratio del proceso %d es:  %f", pcb->contexto_de_ejecucion->pid, rr);
	return rr;
}

void calcular_estimacion(t_pcb* pcb){
	time_t tiempo_actual = time(NULL);
	double rafaga = difftime(tiempo_actual, pcb->tiempo_ingreso_exec);
	uint16_t nueva_estimacion = (1-HRRN_ALFA) * rafaga * 1000 + HRRN_ALFA * pcb->estimado_proxima_rafaga;
	pcb->estimado_proxima_rafaga = nueva_estimacion;
	log_info(logger, "La estimacion para el proceso %d es: %d", pcb->contexto_de_ejecucion->pid, nueva_estimacion);
}

void manejar_io(t_pcb* pcb, int tiempo){
	log_info(logger_obligatorio,"PID: %d - Bloqueado por: %s", pcb->contexto_de_ejecucion->pid,"IO");
	log_info(logger_obligatorio,"PID: %d - Ejecuta IO: %d", pcb->contexto_de_ejecucion->pid, tiempo);

	pthread_t hilo_io;
	t_manejo_io* args = malloc(sizeof(t_manejo_io));
	args->pcb = pcb;
	args->tiempo = tiempo;
	pthread_create(&hilo_io, NULL, (void*)exec_io, (void*)args);
	pthread_detach(hilo_io);
}

void exec_io(void* void_arg){
	t_manejo_io* args = (t_manejo_io*) void_arg;
	t_pcb* pcb =  args->pcb;
	int tiempo = args->tiempo;
	log_info(logger, "ejecutando IO por tiempo: %d", tiempo);
	usleep(tiempo * 1000000);
	safe_pcb_add(cola_block, pcb, &mutex_cola_block);
	sem_post(&sem_block_return);
}

void manejar_wait(t_pcb* pcb, char* recurso){
	t_recurso* recursobuscado= buscar_recurso(recurso);
	if(recursobuscado->id == -1){
		log_error(logger, "No existe el recurso solicitado: %s", recurso);
		pcb->contexto_de_ejecucion->motivo_exit = RECURSO_INEXISTENTE;
		safe_pcb_add(cola_exit,pcb, &mutex_cola_exit);
		sem_post(&sem_exit);
		sem_post(&sem_exec);
	}else{
		recursobuscado->instancias --;
		log_info(logger_obligatorio,"PID: %d - Wait: %s - Instancias: %d", pcb->contexto_de_ejecucion->pid,recurso,recursobuscado->instancias);
		if(recursobuscado->instancias < 0){
			cambiar_estado(pcb, BLOCK);
			log_info(logger_obligatorio,"PID: %d - Bloqueado por: %s", pcb->contexto_de_ejecucion->pid,recurso);
			calcular_estimacion(pcb);
			safe_pcb_add(recursobuscado->cola_block_asignada, pcb, &recursobuscado->mutex_asignado);
			sem_post(&sem_exec);
		}else{
			safe_pcb_add(cola_exec, pcb, &mutex_cola_exec);
			send_contexto_ejecucion(pcb->contexto_de_ejecucion, fd_cpu);
		}
	}
}

void manejar_signal(t_pcb* pcb, char* recurso){
	t_recurso* recursobuscado = buscar_recurso(recurso);
	if(recursobuscado->id == -1){
		log_error(logger, "No existe el recurso solicitado: %s",recurso);
		pcb->contexto_de_ejecucion->motivo_exit = RECURSO_INEXISTENTE;
		safe_pcb_add(cola_exit,pcb, &mutex_cola_exit);
		sem_post(&sem_exit);
		sem_post(&sem_exec);
	}else{
		recursobuscado->instancias++;
		log_info(logger_obligatorio,"PID: %d - Signal: %s - Instancias: %d", pcb->contexto_de_ejecucion->pid,recurso,recursobuscado->instancias);
		if(recursobuscado->instancias <= 0){
			t_pcb* pcb = safe_pcb_remove(recursobuscado->cola_block_asignada, &recursobuscado->mutex_asignado);
			safe_pcb_add(cola_block, pcb,&mutex_cola_block);
			sem_post(&sem_block_return);
		}
		safe_pcb_add(cola_exec, pcb, &mutex_cola_exec);
		send_contexto_ejecucion(pcb->contexto_de_ejecucion, fd_cpu);
	}
}

t_recurso* buscar_recurso(char* recurso){
	int longitudLista = list_size(lista_recursos);
	t_recurso* recursobuscado;
	for(int i = 0; i < longitudLista; i++){
		recursobuscado = list_get(lista_recursos, i);
		if (strcmp(recursobuscado->recurso, recurso) == 0){
			return recursobuscado;
		}
	}
	recursobuscado->id=-1;
	return recursobuscado;
}

void manejar_create_segment(t_pcb* pcb, int cliente_socket, int id_segmento, int tamanio){
	send_create_segment(pcb->contexto_de_ejecucion->pid, id_segmento, tamanio, fd_memoria);
//				recibir de memoria respuesta del create_segment
	t_segment_response respuesta = recv_segment_response(fd_memoria);
	log_info(logger, "recibi la respuesta de memoria");
	switch(respuesta){
	case SEGMENT_CREATED:
		int base_nuevo_segmento = recv_base_segmento(fd_memoria);
		log_info(logger_obligatorio, "PID: %d - Crear Segmento - Id: %d - Tamaño: %d", pcb->contexto_de_ejecucion->pid, id_segmento, tamanio);
		log_info(logger, "recibi de memoria un segmento de base %d", base_nuevo_segmento);
		t_segmento* segmento_nuevo = malloc(sizeof(t_segmento));
		segmento_nuevo->id = id_segmento;
		segmento_nuevo->tamanio = tamanio;
		segmento_nuevo->base = base_nuevo_segmento;
		list_add(pcb->contexto_de_ejecucion->tabla_de_segmentos, segmento_nuevo);
		log_info(logger, "segmento creado, continuando ejecucion");
		safe_pcb_add(cola_exec, pcb, &mutex_cola_exec);
		send_contexto_ejecucion(pcb->contexto_de_ejecucion, cliente_socket);
		break;
	case OUT_OF_MEM:
		log_info(logger, "memoria insuficiente para la creacion del segmento");
		pcb->contexto_de_ejecucion->motivo_exit = OUT_OF_MEMORY;
		safe_pcb_add(cola_exit, pcb, &mutex_cola_exit);
		sem_post(&sem_exit);
		sem_post(&sem_exec);
		break;
	case COMPACT:
		log_info(logger_obligatorio, "Compactación: Esperando Fin de Operaciones de FS");
		sem_wait(&ongoing_fs_mem_op);
//					 - avisar a memoria que compacte.
		log_info(logger_obligatorio, "Compactación: Se solicitó compactación");
		send_iniciar_compactacion(fd_memoria);
//		- recibir de memoria las tablas de segmentos actualizadas post compact
		t_list* ts_wrappers = recv_ts_wrappers(fd_memoria);
		log_info(logger, "recibi la ts_wrappers actualizada de tamaño %d", list_size(ts_wrappers));
		log_info(logger_obligatorio, "Se finalizó el proceso de compactación");
		sem_post(&ongoing_fs_mem_op);
//		- actualizar la tabla de segmentos de TODOS (!) los pcb O.o
		safe_pcb_add(cola_exec, pcb, &mutex_cola_exec);
		actualizar_ts_de_pcbs(ts_wrappers);
		pcb = safe_pcb_remove(cola_exec, &mutex_cola_exec);
//		- mandarle memoria aviso de create_segment.
		manejar_create_segment(pcb, cliente_socket, id_segmento, tamanio);
		break;
	default: log_info(logger, "no entendi el segment response de memoria"); break;
	}
}

t_archivo* get_archivo_global(char* nombre_archivo){
	for(int i = 0; i < list_size(archivos_abiertos); i++){
		t_archivo* archivo = list_get(archivos_abiertos, i);
		if(strcmp(archivo->nombre_archivo, nombre_archivo) == 0){
			return archivo;
		}
	}
	return NULL;
}

t_archivo* get_archivo_pcb(char* nombre_archivo, t_pcb* pcb){
	for(int i = 0; i < list_size(pcb->archivos_abiertos); i++){
		t_archivo* archivo = list_get(pcb->archivos_abiertos, i);
		if(strcmp(archivo->nombre_archivo, nombre_archivo) == 0){
			return archivo;
		}
	}
	return NULL;
}

bool archivo_is_opened(char* nombre_archivo){
	return get_archivo_global(nombre_archivo) != NULL;
}

t_archivo* archivo_create(char* nombre_archivo){
	t_archivo* archivo = malloc(sizeof(t_archivo));
	archivo->nombre_archivo = nombre_archivo;
	archivo->puntero = 0;
	archivo->cola_block_asignada = list_create();
	pthread_mutex_init(&archivo->mutex_asignado, NULL);
	return archivo;
}

t_archivo* quitar_archivo_de_tabla_proceso(char* nombre_archivo, t_pcb* pcb){
	t_archivo* archivo = get_archivo_pcb(nombre_archivo, pcb);
	list_remove_element(pcb->archivos_abiertos, archivo);
	log_info(logger, "al proceso %d le quedan %d archivos abiertos", pcb->contexto_de_ejecucion->pid, list_size(pcb->archivos_abiertos));
	return archivo;
}


void ejecutar_f_open(char* nombre_archivo_open, t_pcb* pcb){
	t_archivo* archivo = archivo_create(nombre_archivo_open);
	bloquear_pcb_por_fs(pcb);
	log_info(logger, "aviso al fs que abra el archivo %s", nombre_archivo_open);
	send_manejar_f_open(archivo->nombre_archivo, fd_filesystem);
	//necesito bloquear hilo conexion cpu para q se quede esperando respuesta de fs
	sem_wait(&fin_f_open);
	list_add(archivos_abiertos, archivo);
	log_info(logger, "ahora hay %d archivos abiertos", list_size(archivos_abiertos));
	list_add(pcb->archivos_abiertos, archivo);
	log_info(logger, "el proceso %d tiene %d archivos abiertos", pcb->contexto_de_ejecucion->pid, list_size(pcb->archivos_abiertos));
}

void bloquear_pcb_por_fs(t_pcb* pcb){
	cambiar_estado(pcb, BLOCK);
	calcular_estimacion(pcb);
	safe_pcb_add(cola_block_fs, pcb, &mutex_cola_block_fs);
	pthread_mutex_lock(&mutex_cola_block_fs);
	int cant_pcbs_block_fs = list_size(cola_block_fs);
	pthread_mutex_unlock(&mutex_cola_block_fs);
	log_info(logger, "agregue un pcb a la cola block_fs y ahora tiene %d pcbs", cant_pcbs_block_fs);
}


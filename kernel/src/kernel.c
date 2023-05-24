#include "../include/kernel.h"

int main(int argc, char **argv) {
	if (argc > 2) {
		return EXIT_FAILURE;
	}
	logger = log_create("kernel.log", "kernel_main", 1, LOG_LEVEL_INFO);
	// TODO: cambiar archivo de logs obligatorios
	logger_obligatorio = log_create("kernel.log", "kernel_obligatorio", 1, LOG_LEVEL_INFO);
	config = config_create(argv[1]);
	if (config == NULL) {
		log_error(logger, "No se encontró el archivo :(");
		exit(1);
	}
	leer_config();

	// Conecto kernel con cpu, memoria y filesystem
	fd_cpu = 0, fd_memoria = 0, fd_filesystem = 0;
	if (!generar_conexiones()) {
		log_error(logger, "Alguna conexion falló :(");
		terminar_programa(logger, config);
		exit(1);
	}

	// Intercambio de mensajes...

//	enviar_mensaje("Hola, Soy Kernel!", fd_filesystem);
//	enviar_mensaje("Hola, Soy Kernel!", fd_memoria);
	enviar_mensaje("Hola, Soy Kernel!", fd_cpu);

	inicializar_variables();
	planificar();

	server_socket = iniciar_servidor(logger, IP, PUERTO);
	while (server_escuchar(server_socket));

	liberar_variables();
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
	string_array_destroy(instancias);
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
//	pthread_t conexion_filesystem;
	pthread_t conexion_cpu;
//	pthread_t conexion_memoria;
//
//	fd_filesystem = crear_conexion(IP_FILESYSTEM, PUERTO_FILESYSTEM);
//	pthread_create(&conexion_filesystem, NULL, (void*) procesar_conexion, (void*) &fd_filesystem);
//	pthread_detach(conexion_filesystem);

	fd_cpu = crear_conexion(IP_CPU, PUERTO_CPU);
	pthread_create(&conexion_cpu, NULL, (void*) procesar_conexion, (void*) &fd_cpu);
	pthread_detach(conexion_cpu);
//
//	fd_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
//	pthread_create(&conexion_memoria, NULL, (void*) procesar_conexion, (void*) &fd_memoria);
//	pthread_detach(conexion_memoria);

//	return fd_filesystem != 0 && fd_cpu != 0 && fd_memoria != 0;
	return fd_cpu != 0;
}

void inicializar_variables() {
	//PCBs
	generador_pid = 1;
	lista_ready = list_create();
	cola_exit = queue_create();
	cola_listos_para_ready = queue_create();
	cola_exec = queue_create();
	cola_block = queue_create();
	lista_recursos = inicializar_recursos();


	//Semaforos
	pthread_mutex_init(&mutex_generador_pid, NULL);
	pthread_mutex_init(&mutex_cola_ready, NULL);
	pthread_mutex_init(&mutex_cola_listos_para_ready, NULL);
	pthread_mutex_init(&mutex_cola_exit, NULL);
	pthread_mutex_init(&mutex_cola_exec, NULL);
	pthread_mutex_init(&mutex_cola_block, NULL);
	sem_init(&sem_multiprog, 0, GRADO_MAX_MULTIPROGRAMACION);
	sem_init(&sem_listos_ready, 0, 0);
	sem_init(&sem_ready, 0, 0);
	sem_init(&sem_exec, 0, 1);
	sem_init(&sem_exit, 0, 0);

}

t_list* inicializar_recursos(){
	t_list* lista = list_create();
	int cantidad_recursos = string_array_size(RECURSOS);
	for(int i = 0; i < cantidad_recursos; i++){
		char* string = RECURSOS[i];
		t_recurso* recurso = malloc(sizeof(t_recurso));
		recurso->recurso = malloc(sizeof(char) * strlen(string) + 1);
		strcpy(recurso->recurso, string);
		t_queue* cola_block = queue_create();
		recurso->id = i;
		recurso->instancias = INSTANCIAS_RECURSOS[i];
		recurso->cola_block_asignada = cola_block;
		pthread_mutex_init(&recurso->mutex_asignado, NULL);
		list_add(lista, recurso);
	}
	return lista;
}

void liberar_variables(){
	log_destroy(logger);
	log_destroy(logger_obligatorio);
	config_destroy(config);
	free(INSTANCIAS_RECURSOS);
	free(RECURSOS);
}

// ------------------ COMUNICACION ------------------

static void procesar_conexion(void* void_args) {
	int *args = (int*) void_args;
	int cliente_socket = *args;

	t_contexto_ejecucion* contexto_recibido;
	t_pcb* pcb;
	char* recurso;

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
		case INSTRUCCIONES_CONSOLA:
			t_list *instrucciones = recv_instrucciones(logger, cliente_socket);
			armar_pcb(instrucciones, cliente_socket);
			break;
		case CAMBIAR_ESTADO:
			log_info(logger, "recibi un aviso de cambio de estado");
			contexto_recibido = recv_cambiar_estado(cliente_socket);
			pcb = safe_pcb_pop(cola_exec, &mutex_cola_exec);
			calcular_estimacion(pcb);
			actualizar_contexto_pcb(pcb, contexto_recibido);
			procesar_cambio_estado(pcb, contexto_recibido->estado);
			sem_post(&sem_exec);
			break;
		case CONTEXTO_EJECUCION:
			contexto_recibido = recv_contexto_ejecucion(cliente_socket);
			pcb = safe_pcb_pop(cola_exec, &mutex_cola_exec);
			actualizar_contexto_pcb(pcb, contexto_recibido);
			recv(cliente_socket, &cop, sizeof(op_code), 0);
			switch(cop){
			case MANEJAR_IO:
				int tiempo = recv_tiempo_io(cliente_socket);
				calcular_estimacion(pcb);
				cambiar_estado(pcb, BLOCK);
				sem_post(&sem_exec);
				manejar_io(pcb, tiempo);
				break;
			case MANEJAR_WAIT:
				recurso = recv_recurso(cliente_socket);
				log_info(logger, "recurso recibido del wait: %s", recurso);
				manejar_wait(pcb, recurso);
				break;
			case MANEJAR_SIGNAL:
				recurso = recv_recurso(cliente_socket);
				manejar_signal(pcb, recurso);
				safe_pcb_push(cola_exec, pcb, &mutex_cola_exec);
				send_contexto_ejecucion(pcb->contexto_de_ejecucion,cliente_socket);
				break;
			default:
				log_error(logger, "Codigo de operacion no reconocido en el server de %s", server_name);
				return;
			}
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
	server_name = "Kernel";
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

// ------------------ PCBS ------------------

t_pcb* pcb_create(t_list* instrucciones, int pid, int cliente_socket) {
	t_pcb *pcb = malloc(sizeof(t_pcb));
	pcb->fd_consola = cliente_socket;
	pcb->registros = NULL;
	pcb->seg_fault = NULL;
	pcb->estimado_proxima_rafaga = ESTIMACION_INICIAL;

	t_contexto_ejecucion* contexto = malloc(sizeof(t_contexto_ejecucion));
	pcb->contexto_de_ejecucion = contexto;
	pcb->contexto_de_ejecucion->pid = pid;
	pcb->contexto_de_ejecucion->program_counter = 0;
	pcb->contexto_de_ejecucion->instrucciones = instrucciones;

	// Hago esto aca para que no rompa el protocolo, despues se tiene que hacer en memoria
	// en vez de list_create() deberiamos tener un send a memoria para que inicialice la tabla
	// despues un recv tabla y ahi se asigna
	t_segmento* segmento = malloc(sizeof(t_segmento));
	segmento->direccion_fisica = 0;
	segmento->id = 0;
	segmento->tamanio_segmentos = 0;
	pcb->contexto_de_ejecucion->tabla_de_segmentos = list_create();
	list_add(pcb->contexto_de_ejecucion->tabla_de_segmentos, segmento);

	pcb->contexto_de_ejecucion->estado = NEW;

	return pcb;
}

// hay que acordarse de agregar frees aca si se cambia la estructura del t_pcb !!!
void pcb_destroy(t_pcb* pcb){
	list_destroy(pcb->contexto_de_ejecucion->instrucciones);
	list_destroy(pcb->contexto_de_ejecucion->tabla_de_segmentos);
	free(pcb->contexto_de_ejecucion);
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
		safe_pcb_push(cola_listos_para_ready, pcb, &mutex_cola_ready);
		sem_post(&sem_listos_ready);
		break;
	case FINISH_EXIT:
		cambiar_estado(pcb, estado_nuevo);
		safe_pcb_push(cola_exit, pcb, &mutex_cola_exit);
		sem_post(&sem_exit);
		break;
	case FINISH_ERROR:
		cambiar_estado(pcb, estado_nuevo);
		safe_pcb_push(cola_exit, pcb, &mutex_cola_exit);
		sem_post(&sem_exit);
		break;
	default: break;
	}
}

void armar_pcb(t_list *instrucciones, int cliente_socket) {
	pthread_mutex_lock(&mutex_generador_pid);
	int pid = generador_pid;
	generador_pid++;
	pthread_mutex_unlock(&mutex_generador_pid);
	t_pcb *pcb = pcb_create(instrucciones, pid, cliente_socket);
	log_info(logger_obligatorio, "Se crea el proceso %d en NEW", pid);
	safe_pcb_push(cola_listos_para_ready, pcb, &mutex_cola_listos_para_ready);
	sem_post(&sem_listos_ready);
}

void actualizar_contexto_pcb(t_pcb* pcb, t_contexto_ejecucion* contexto){
	pcb->contexto_de_ejecucion->program_counter = contexto->program_counter;
	*(pcb->contexto_de_ejecucion->tabla_de_segmentos) = *(contexto->tabla_de_segmentos);
	pcb->contexto_de_ejecucion->motivo_exit = contexto->motivo_exit;
	pcb->contexto_de_ejecucion->motivo_block = contexto->motivo_block;

	//contexto_destroy(contexto);
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
	pthread_create(&hilo_exit, NULL, (void*) exit_pcb, NULL);
	pthread_create(&hilo_ready, NULL, (void*) ready_pcb, NULL);
	pthread_detach(hilo_exit);
	pthread_detach(hilo_ready);
}

void exit_pcb(void) {
	while (1)
	{
		sem_wait(&sem_exit);
		t_pcb *pcb = safe_pcb_pop(cola_exit, &mutex_cola_exit);
		char* motivo = motivo_exit_to_string(pcb->contexto_de_ejecucion->motivo_exit);
		log_info(logger_obligatorio, "Finaliza el proceso %d - Motivo: %s", pcb->contexto_de_ejecucion->pid, motivo);
		enviar_mensaje("Fin del proceso", pcb->fd_consola);
		pcb_destroy(pcb);
	}
}

void ready_pcb(void) {
	while (1) {
		sem_wait(&sem_listos_ready);
		t_pcb *pcb = safe_pcb_pop(cola_listos_para_ready, &mutex_cola_listos_para_ready);
		sem_wait(&sem_multiprog);
		set_pcb_ready(pcb);
		sem_post(&sem_ready);
	}
}

t_pcb *safe_pcb_pop(t_queue *queue, pthread_mutex_t *mutex)
{
	t_pcb *pcb;
	pthread_mutex_lock(mutex);
	pcb = queue_pop(queue);
	pthread_mutex_unlock(mutex);
	return pcb;
}

void safe_pcb_push(t_queue *queue, t_pcb *pcb, pthread_mutex_t *mutex)
{
	pthread_mutex_lock(mutex);
	queue_push(queue, pcb);
	pthread_mutex_unlock(mutex);
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
	// no safe_pcb_push
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
        t_pcb *pcb = (t_pcb *)list_remove(list, 0);
        int *valor = &(pcb->contexto_de_ejecucion->pid);
        list_add(lista_de_pids, valor);
        list_add(list, pcb);
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
		sem_post(&sem_multiprog);
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
	safe_pcb_push(cola_exec, pcb, &mutex_cola_exec);
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
	safe_pcb_push(cola_block, pcb, &mutex_cola_block);
	usleep(tiempo * 1000000);
	safe_pcb_pop(cola_block, &mutex_cola_block);
	safe_pcb_push(cola_listos_para_ready, pcb, &mutex_cola_listos_para_ready);
	sem_post(&sem_listos_ready);
}

void manejar_wait(t_pcb* pcb, char* recurso){
	t_recurso* recursobuscado= buscar_recurso(recurso);
	if(recursobuscado->id == -1){
		log_error(logger, "No existe el recurso solicitado: %s", recurso);
		safe_pcb_push(cola_exit,pcb, &mutex_cola_exit);
		sem_post(&sem_exit);
		sem_post(&sem_exec);
	}else{
		recursobuscado->instancias --;
		log_info(logger_obligatorio,"PID: %d - Wait: %s - Instancias: %d", pcb->contexto_de_ejecucion->pid,recurso,recursobuscado->instancias);
		if(recursobuscado->instancias < 0){
			log_info(logger_obligatorio,"PID: %d - Bloqueado por: %s", pcb->contexto_de_ejecucion->pid,recurso);
			calcular_estimacion(pcb);
			safe_pcb_push(recursobuscado->cola_block_asignada, pcb, &recursobuscado->mutex_asignado);
			sem_post(&sem_exec);
		}else{
			safe_pcb_push(cola_exec, pcb, &mutex_cola_exec);
			send_contexto_ejecucion(pcb->contexto_de_ejecucion, fd_cpu);
		}
	}
}

void manejar_signal(t_pcb* pcb, char* recurso){
	t_recurso* recursobuscado = buscar_recurso(recurso);
	if(recursobuscado->id == -1){
		log_error(logger, "No existe el recurso solicitado: %s",recurso);
		safe_pcb_push(cola_exit,pcb, &mutex_cola_exit);
		sem_post(&sem_exit);
	}else{
		recursobuscado->instancias++;
		log_info(logger_obligatorio,"PID: %d - Signal: %s - Instancias: %d", pcb->contexto_de_ejecucion->pid,recurso,recursobuscado->instancias);
		if(recursobuscado->instancias <= 0){
			t_pcb* pcb = safe_pcb_pop(recursobuscado->cola_block_asignada, &recursobuscado->mutex_asignado);
			safe_pcb_push(cola_listos_para_ready, pcb,&mutex_cola_listos_para_ready);
			sem_post(&sem_listos_ready);
		}
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


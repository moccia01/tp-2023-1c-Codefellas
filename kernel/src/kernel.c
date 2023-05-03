#include "../include/kernel.h"

int main(void) {
	logger = log_create("kernel.log", "kernel_main", 1, LOG_LEVEL_INFO);
	logger_obligatorio = log_create("kernel_obligatorio.log", "kernel_obligatorio", 1, LOG_LEVEL_INFO);
	config = config_create("kernel.config");
	lista_pcbs = list_create();

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

	enviar_mensaje("Hola, Soy Kernel!", fd_filesystem);
	enviar_mensaje("Hola, Soy Kernel!", fd_memoria);
	enviar_mensaje("Hola, Soy Kernel!", fd_cpu);

	inicializar_variables();
	planificar();

	server_socket = inicializar_servidor();
	while (server_escuchar(server_socket));

	terminar_programa(logger, config);
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
	// HRRN_ALFA;
	GRADO_MAX_MULTIPROGRAMACION = config_get_int_value(config, "GRADO_MAX_MULTIPROGRAMACION");
	log_info(logger, "Grado multi prog: %d", GRADO_MAX_MULTIPROGRAMACION);
	// RECURSOS;
	// INSTANCIAS_RECURSOS;
}

void asignar_algoritmo(char *algoritmo) {
	if (strcmp(algoritmo, "FIFO")) {
		ALGORITMO_PLANIFICACION = FIFO;
	} else if (strcmp(algoritmo, "FIFO")) {
		ALGORITMO_PLANIFICACION = HRRN;
	}
}

int inicializar_servidor() {
	int fd_kernel = iniciar_servidor(logger, IP, PUERTO);
	return fd_kernel;
}

bool generar_conexiones() {
	fd_filesystem = crear_conexion(IP_FILESYSTEM, PUERTO_FILESYSTEM);
	fd_cpu = crear_conexion(IP_CPU, PUERTO_CPU);
	fd_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);

	return fd_filesystem != 0 && fd_cpu != 0 && fd_memoria != 0;
}

void inicializar_variables() {
	//PCBs
	lista_pcbs = list_create();
	generador_pid = 1;
	cola_ready = queue_create();
	cola_exit = queue_create();
	cola_listos_para_ready = queue_create();
	cola_exec = queue_create();

	//Semaforos
	pthread_mutex_init(&mutex_generador_pid, NULL);
	pthread_mutex_init(&mutex_cola_ready, NULL);
	pthread_mutex_init(&mutex_cola_listos_para_ready, NULL);
	sem_init(&sem_multiprog, 0, GRADO_MAX_MULTIPROGRAMACION);
	sem_init(&sem_listos_ready, 0, 0);
	sem_init(&sem_ready, 0, 0);
	sem_init(&sem_exec, 0, 0);

}

// ------------------ COMUNICACION ------------------

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
		case INSTRUCCIONES_CONSOLA:
			t_list *instrucciones = recv_instrucciones(logger, cliente_socket);
			armar_pcb(instrucciones);
			break;
		default:
			log_error(logger, "Algo anduvo mal en el server de %s",
					server_name);
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
	char *server_name = "Kernel";
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

void armar_pcb(t_list *instrucciones) {
	t_proceso *proceso = malloc(sizeof(t_proceso));
	pthread_mutex_lock(&mutex_generador_pid);
	int pid = generador_pid;
	generador_pid++;
	pthread_mutex_unlock(&mutex_generador_pid);
	proceso->instrucciones = instrucciones;
	t_pcb *pcb = pcb_create(proceso, pid);
	log_info(logger, "Se crea el proceso <%d> en NEW", pid);
	// mutex
	list_add(lista_pcbs, pcb);
	queue_push(cola_listos_para_ready, pcb);
	sem_post(&sem_listos_ready);
}

t_pcb* pcb_create(t_proceso *proceso, int pid) {
	t_pcb *pcb = malloc(sizeof(t_pcb));

	pcb->contexto_de_ejecucion.estado = NEW;
	//pcb->instrucciones = proceso->instrucciones;
	pcb->contexto_de_ejecucion.pid = pid;
	// pcb->socket_consola = socket;
	//pcb->program_counter = 0;
	pcb->interrupcion = false;
	pcb->contexto_de_ejecucion.tabla_de_segmentos = NULL;
	pcb->contexto_de_ejecucion.pid = pid;
	pcb->contexto_de_ejecucion.program_counter = 0;
	pcb->contexto_de_ejecucion.instrucciones = proceso->instrucciones;
	//  pcb->con_desalojo = false;
	// pcb->tamanio_segmentos = proceso->segmentos;

	return pcb;
}

void cambiar_estado(t_pcb *pcb, estado_proceso nuevo_estado) {
	char *nuevo_estado_string = strdup(estado_to_string(nuevo_estado));
	char *estado_anterior_string = strdup(
			estado_to_string(pcb->contexto_de_ejecucion.estado));
	log_info(logger, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>",
			pcb->contexto_de_ejecucion.pid, estado_anterior_string,
			nuevo_estado_string);
	free(estado_anterior_string);
	free(nuevo_estado_string);
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
	log_info(logger, "exit_pcb");
}

void ready_pcb(void) {
	log_info(logger, "ready_pcb");
	while (1) {
		sem_wait(&sem_listos_ready);
		t_pcb *pcb = queue_pop(cola_listos_para_ready);
		sem_wait(&sem_multiprog);
		setear_pcb_ready(pcb);
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

void setear_pcb_ready(t_pcb *pcb) {
	cambiar_estado(pcb, READY);
	queue_push(cola_ready, pcb);
	pthread_mutex_lock(&mutex_cola_ready);
	log_cola_ready();
	pthread_mutex_lock(&mutex_cola_ready);
}

void log_cola_ready(){
	t_list *lista_a_loguear = pcb_queue_to_pid_list(cola_ready);
	char *lista = list_to_string(lista_a_loguear);
	log_info(logger_obligatorio, "Cola Ready %s: [%s]", algoritmo_to_string(ALGORITMO_PLANIFICACION), lista);
	list_destroy(lista_a_loguear);
	free(lista);
}

t_list *pcb_queue_to_pid_list(t_queue *queue)
{
    t_list *lista = list_create();
    for (int i = 0; i < queue_size(queue); i++)
    {
        t_pcb *pcb = (t_pcb *)queue_pop(queue);
        int *valor = &pcb->contexto_de_ejecucion.pid;
        list_add(lista, valor);
        queue_push(queue, pcb);
    }
    return lista;
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

	switch (ALGORITMO_PLANIFICACION) {
	case FIFO:
		pthread_create(&hilo_corto_plazo, NULL, (void*) planificar_FIFO, NULL);
		pthread_detach(hilo_corto_plazo);
		break;
	case HRRN:
		pthread_create(&hilo_corto_plazo, NULL, (void*) planificar_HRRN, NULL);
		pthread_detach(hilo_corto_plazo);
		break;
	default:
		log_error(logger, "No se reconocio el algoritmo de planifacion");
	}
}

void planificar_FIFO() {
	while (1) {
		sem_wait(&sem_ready);
		t_pcb *pcb = queue_pop(cola_ready);
		sem_post(&sem_multiprog);
		sem_wait(&sem_exec);
		cambiar_estado(pcb, EXEC);
		run_pcb(pcb);
	}
}

void run_pcb(t_pcb* pcb){
	send_cambiar_estado(pcb->contexto_de_ejecucion, fd_cpu);
}

void planificar_HRRN(){

}

#include "../include/comunicacion.h"

static void procesar_conexion(void *void_args) {
	t_procesar_conexion_args *args = (t_procesar_conexion_args*) void_args;
	t_log *logger = args->log;
	t_config* config = args->cfg;
	int cliente_socket = args->fd;
	char *server_name = args->server_name;
	free(args);

	op_code cop;
	while (cliente_socket != -1) {
        if (recv(cliente_socket, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
            log_info(logger, "El cliente se desconecto de %s server", server_name);
            return;
        }
		switch (cop) {
		case MENSAJE:
			recibir_mensaje(logger, cliente_socket);
			break;
		case PAQUETE:
			t_list* paquete_recibido = recibir_paquete(cliente_socket);
			log_info(logger, "Recibí un paquete");
			break;
		case INSTRUCCIONES_CONSOLA:
			t_list* instrucciones = recv_instrucciones(logger, cliente_socket);
			//	armar_pcb(instrucciones);
			log_info(logger, "Recibí las instrucciones de la consola.");
			armar_pcb(instrucciones);
			planificar(config);
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

int server_escuchar(t_log *logger,t_config* config, int server_socket) {
	char *server_name = "Kernel";
	int cliente_socket = esperar_cliente(logger, server_name, server_socket);

	if (cliente_socket != -1) {
		pthread_t hilo;
		t_procesar_conexion_args *args = malloc(sizeof(t_procesar_conexion_args));
		args->log = logger;
		args->cfg = config;
		args->fd = cliente_socket;
		args->server_name = server_name;
		pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) args);
		pthread_detach(hilo);
		return 1;
	}
	return 0;
}

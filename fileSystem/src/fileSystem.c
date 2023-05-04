#include "../include/fileSystem.h"

int main(void) {
	logger = log_create("filesystem.log", "filesystem_main", 1, LOG_LEVEL_INFO);
	config = config_create("filesystem.config");

	// Inicio de servidor
	fd_filesystem = iniciar_servidor(logger, IP, PUERTO);

	// Conexion Kernel
	pthread_t conexion_memoria;
	pthread_create(&conexion_memoria, NULL, (void*) server_escuchar, NULL);
	pthread_detach(conexion_memoria);

	// Conecto CPU con memoria
	fd_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
	enviar_mensaje("Hola, soy Filesystem.", fd_memoria);

	terminar_programa(logger, config);
	return 0;
}

void leer_config(){
	IP = config_get_string_value(config, "IP_ESCUCHA");
	PUERTO = config_get_string_value(config, "PUERTO_ESCUCHA");
	IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
	PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");
}

void inicializar_variables(){

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

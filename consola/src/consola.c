#include "../include/consola.h"

int main(int argc, char **argv) {
	logger = log_create("consola.log", "consola_main", true, LOG_LEVEL_INFO);
	logger_obligatorio = log_create("consola_obligatorio.log", "consola_obligatorio", true, LOG_LEVEL_INFO);

	if (argc > 3) {
		return EXIT_FAILURE;
	}

	config = config_create(argv[1]);
	if (config == NULL) {
		log_error(logger, "No se encontró el archivo :(");
		exit(1);
	}
	leer_config();

	// Conexion Kernel
	fd_kernel = crear_conexion(IP_KERNEL, PUERTO_KERNEL);
	enviar_mensaje("Hola, soy consola.", fd_kernel);

	t_list *lista_de_insturcciones = leer_instrucciones(argv[2], logger);
	send_instrucciones(fd_kernel, lista_de_insturcciones);

	// La consola quedará a la espera del mensaje del Kernel que indique la finalización del proceso.
	recibir_mensaje(logger, fd_kernel);

	terminar_programa(logger, config);
	liberar_conexion(fd_kernel);
	return 0;
}

void leer_config(){
	IP_KERNEL = config_get_string_value(config, "IP_KERNEL");
	// El enunciado dice q el puerto es numerico, pero la funcion getaddrinfo recibe char* y no puedo castear de int a char* :(
	PUERTO_KERNEL = config_get_string_value(config, "PUERTO_KERNEL");
}

t_list* leer_instrucciones(char *path, t_log *logger) {
	t_list *lista_de_instrucciones = list_create();
	FILE *pseudocodigo;
	char buffer[MAX_LINE_LENGTH];
	char *instruccion_token;
	char *parametro1;
	char *parametro2;
	char *parametro3;
	t_instruccion instruccion;

	pseudocodigo = fopen(path, "r");

	if (pseudocodigo == NULL) {
		log_error(logger, "No se pudo abrir el archivo.\n");
		exit(1);
	}

	while (fgets(buffer, MAX_LINE_LENGTH, pseudocodigo) != NULL) {
		// Eliminar el carácter de salto de línea del final de la línea
		buffer[strcspn(buffer, "\n")] = '\0';

		// Obtener la instrucción y los parámetros de la línea
		instruccion_token = strtok(buffer, " ");
		parametro1 = strtok(NULL, " ");
		parametro2 = strtok(NULL, " ");
		parametro3 = strtok(NULL, " ");

		instruccion.instruccion = instruccion_token; // @suppress("Field cannot be resolved")
		instruccion.parametro1 = parametro1;
		instruccion.parametro2 = parametro2;
		instruccion.parametro3 = parametro3;

		list_add(lista_de_instrucciones, &instruccion);
	}

	// Cerrar el archivo
	fclose(pseudocodigo);

	return lista_de_instrucciones;
}

//static void procesar_conexion(){
//
//	op_code cop;
//	while (fd_kernel != -1) {
//        if (recv(fd_kernel, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
//            return;
//        }
//		switch (cop) {
//		case MENSAJE:
//			recibir_mensaje(logger, fd_kernel);
//			break;
//		default:
//			log_error(logger, "Algo anduvo mal en el server de consola");
//			return;
//		}
//	}
//
//	log_warning(logger, "El cliente se desconecto de la consola");
//	return;
//}



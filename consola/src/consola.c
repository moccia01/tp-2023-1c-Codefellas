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

//	// Conexion Kernel
//	fd_kernel = crear_conexion(IP_KERNEL, PUERTO_KERNEL);
//	enviar_mensaje("Hola, soy consola.", fd_kernel);

	t_list lista_de_instrucciones = leer_instrucciones(argv[2], logger);
	t_instruccion* instruccion = list_get(&lista_de_instrucciones, 0);
	log_info(logger, "la primera instruccion es %s", instruccion->instruccion);
	instruccion = list_get(&lista_de_instrucciones, 1);
	log_info(logger, "la segunda instruccion es %s", instruccion->instruccion);
	instruccion = list_get(&lista_de_instrucciones, 2);
	log_info(logger, "la tercera instruccion es %s", instruccion->instruccion);
	log_info(logger, "tamaño de list = %d", list_size(&lista_de_instrucciones));
//	send_instrucciones(fd_kernel, lista_de_instrucciones);

	// La consola quedará a la espera del mensaje del Kernel que indique la finalización del proceso.
	pthread_t conexion_kernel;
	pthread_create(&conexion_kernel, NULL, (void*) procesar_conexion, NULL);
	pthread_join(conexion_kernel, NULL);

	terminar_programa(logger, config);
	liberar_conexion(fd_kernel);
	return 0;
}

void leer_config(){
	IP_KERNEL = config_get_string_value(config, "IP_KERNEL");
	// El enunciado dice q el puerto es numerico, pero la funcion getaddrinfo recibe char* y no puedo castear de int a char* :(
	PUERTO_KERNEL = config_get_string_value(config, "PUERTO_KERNEL");
}

t_list leer_instrucciones(char *path, t_log *logger) {
	t_list lista_de_instrucciones = *list_create();
	FILE *pseudocodigo;
	char buffer[MAX_LINE_LENGTH];

	pseudocodigo = fopen(path, "r");

	if (pseudocodigo == NULL) {
		log_error(logger, "No se pudo abrir el archivo.\n");
		exit(1);
	}

	while (fgets(buffer, MAX_LINE_LENGTH, pseudocodigo) != NULL) {
		// Eliminar el carácter de salto de línea del final de la línea
		buffer[strcspn(buffer, "\n")] = '\0';

		char *instruccion_token = malloc(MAX_LINE_LENGTH * sizeof(char));
		char *parametro1 = malloc(MAX_LINE_LENGTH * sizeof(char));
		char *parametro2 = malloc(MAX_LINE_LENGTH * sizeof(char));
		char *parametro3 = malloc(MAX_LINE_LENGTH * sizeof(char));
		t_instruccion* instruccion = malloc(sizeof(t_instruccion));

		// Esta pija rompe y no se porque pero puede que aca este la solucion:
		// https://github.com/sisoputnfrba/foro/issues/2600
		// usar string_split()?

		// Obtener la instrucción y los parámetros de la línea
		instruccion_token = strtok(buffer, " ");
		parametro1 = strtok(NULL, " ");
		parametro2 = strtok(NULL, " ");
		parametro3 = strtok(NULL, " ");

		instruccion->instruccion = instruccion_token; // @suppress("Field cannot be resolved")
		instruccion->parametro1 = parametro1;
		instruccion->parametro2 = parametro2;
		instruccion->parametro3 = parametro3;

		log_info(logger, "inst: %s", instruccion->instruccion);
		int pos = list_add(&lista_de_instrucciones, instruccion);

		t_instruccion* inst = list_get(&lista_de_instrucciones, pos);
		log_info(logger, "inst pos %d: %s", pos, inst->instruccion);
	}

	// Cerrar el archivo
	fclose(pseudocodigo);

	return lista_de_instrucciones;
}

static void procesar_conexion(){
	op_code cop;
	while (fd_kernel != -1) {
        if (recv(fd_kernel, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
            return;
        }
		switch (cop) {
		case MENSAJE:
			recibir_mensaje(logger, fd_kernel);
			break;
		default:
			log_error(logger, "Algo anduvo mal en el server de consola");
			return;
		}
	}

	log_warning(logger, "El cliente se desconecto de la consola");
	return;
}



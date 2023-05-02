#include "../include/consola.h"

int main(int argc, char **argv) {

	logger = log_create("consola.log", "consola_main", true,
			LOG_LEVEL_INFO);

	if (argc > 3) {
		return EXIT_FAILURE;
	}

	config = config_create(argv[1]); //Tira warning aca xq no lo usamos nunca

	//FILE* pseudo_code = fopen(argv[2], "r");

	if (config == NULL) {
		log_error(logger, "No se encontró el archivo :(");
		exit(1);
	}


	char *IP_KERNEL = config_get_string_value(config, "IP_KERNEL");
	// El enunciado dice q el puerto es numerico, pero la funcion getaddrinfo recibe char* y no puedo castear de int a char* :(
	char *PUERTO_KERNEL = config_get_string_value(config, "PUERTO_KERNEL");

	// Conexion Kernel
	int fd_kernel = crear_conexion(IP_KERNEL, PUERTO_KERNEL);
	enviar_mensaje("Hola, soy consola.", fd_kernel);

	t_list *lista_de_insturcciones_pseudocodigo = leer_instrucciones(argv[2], logger);

	send_instrucciones(fd_kernel, lista_de_insturcciones_pseudocodigo);

	terminar_programa(logger, config);
	liberar_conexion(fd_kernel);

	return 0;
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

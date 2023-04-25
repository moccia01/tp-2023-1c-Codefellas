#include "../include/kernel.h"

int main(void) {
	t_log* logger = log_create("kernel.log", "kernel_main", 1, LOG_LEVEL_INFO);
	t_config* config = config_create("kernel.config");

	if(config == NULL){
		log_error(logger, "No se encontró el archivo :(");
		exit(1);
	}

	// Inicio de servidor
	char* IP = config_get_string_value(config, "IP_ESCUCHA");
	char* PUERTO = config_get_string_value(config, "PUERTO_ESCUCHA");
	int fd_kernel = iniciar_servidor(logger, IP, PUERTO);

	// Conexion consola
	int fd_consola = esperar_cliente(logger, fd_kernel);
	int cod_op = recibir_operacion(fd_consola);
	if(cod_op != MENSAJE){
		log_error(logger, "no se q paso pero exploto\n");
		exit(1);
	}
	recibir_mensaje(logger, fd_consola);
	t_list* instrucciones = recv_instrucciones(fd_consola);


	// Conecto kernel con cpu, memoria y filesystem
	int fd_cpu = 0, fd_memoria = 0, fd_filesystem = 0;
	if(!generar_conexiones(logger, config, &fd_cpu, &fd_memoria, &fd_filesystem)){
		log_error(logger, "Alguna conexion falló :(");
		terminar_programa(logger, config);
		exit(1);
	}

	// Intercambio de mensajes...

	enviar_mensaje("Hola, Soy Kernel!", fd_filesystem);
	enviar_mensaje("Hola, Soy Kernel!", fd_memoria);
	enviar_mensaje("Hola, Soy Kernel!", fd_cpu);

	terminar_programa(logger, config);
	liberar_conexion(fd_consola);
	return 0;
}

t_pcb *pcb_create(t_proceso *proceso, int pid, int socket)
{
    t_pcb *pcb = malloc(sizeof(t_pcb));

    pcb->estado = NEW;
    pcb->instrucciones = proceso->instrucciones;
    pcb->pid = pid;
   // pcb->socket_consola = socket;
    pcb->program_counter = 0;
    pcb->interrupcion = false;
    pcb->registros.ax = 0;
    pcb->registros.bx = 0;
    pcb->registros.cx = 0;
    pcb->registros.dx = 0;
    pcb->pagina_fault = NULL;
    pcb->tabla_de_segmentos = NULL;
   //  pcb->con_desalojo = false;
    // pcb->tamanio_segmentos = proceso->segmentos;

    return pcb;
}

//void cambiar_estado(t_log* logger, t_pcb *pcb, estado_proceso nuevo_estado)
//{
//	char *nuevo_estado_string = strdup(estado_to_string(nuevo_estado));
//	char *estado_anterior_string = strdup(estado_to_string(pcb->estado));
//	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", pcb->pid, estado_anterior_string, nuevo_estado_string);
//	free(estado_anterior_string);
//	free(nuevo_estado_string);
//}

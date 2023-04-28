#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/list.h>

void planificar(t_config*);
void planificar_largo_plazo(t_config*);
void planificar_corto_plazo(t_config*);

#endif /* PLANIFICADOR_H_ */

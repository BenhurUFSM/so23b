#ifndef SO_H
#define SO_H

// so
// o sistema operacional


typedef struct so_t so_t;

#include "memoria.h"
#include "cpu.h"
#include "console.h"

so_t *so_cria(cpu_t *cpu, mem_t *mem, console_t *console);
void so_destroi(so_t *self);

#endif // SO_H

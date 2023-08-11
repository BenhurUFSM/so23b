#ifndef CONTROLE_H
#define CONTROLE_H

// controle
// unidade de controle, controla o hardware simulado
// em especial, contém o laço de execução de instruções
// concentra os dispositivos de hardware


typedef struct controle_t controle_t;

#include "memoria.h"
#include "cpu.h"
#include "console.h"
#include "relogio.h"

controle_t *controle_cria(cpu_t *cpu, console_t *console, relogio_t *relogio);
void controle_destroi(controle_t *self);

// o laço principal da simulação
void controle_laco(controle_t *self);

#endif // CONTROLE_H

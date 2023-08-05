#ifndef CONTROLE_H
#define CONTROLE_H

// controle
// unidade de controle, controla o hardware simulado
// em especial, contém o laço de execução de instruções
// concentra os dispositivos de hardware


#define MEM_TAM 2000        // tamanho da memória principal

typedef struct controle_t controle_t;

#include "memoria.h"
#include "cpu.h"
#include "es.h"

controle_t *controle_cria(void);
void controle_destroi(controle_t *self);

// o laço principal da simulação
void controle_laco(controle_t *self);

// funções de acesso aos componentes do hardware
mem_t *controle_mem(controle_t *self);
cpu_t *controle_cpu(controle_t *self);
es_t *controle_es(controle_t *self);

#endif // CONTROLE_H

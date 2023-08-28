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

// Chamadas de sistema (a chamada está em A quando executa CHAMASIS)

// lê um caractere do dispositivo de entrada
// retorna em A: o caractere lido ou um código de erro negativo
#define SO_LE          1

// escreve o caractere em X no dispositivo de saída
// retorna em A: 0 se OK ou um código de erro negativo
#define SO_ESCR        2

#endif // SO_H

#include "processo.h"
#include "cpu.h"
#include <stdlib.h>

struct processo_t {
   estado_processo estado_processo;
   int pid;
   char *nome_do_executavel;
   registros_t *estado_cpu;
   bool livre;
};

struct registros_t {
    int PC;
    int A;
    int X;
    int complemento;
    int erro;
    cpu_modo_t modo;
};
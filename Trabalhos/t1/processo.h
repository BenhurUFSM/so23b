#ifndef PROCESSO_H
#define PROCESSO_H

typedef struct processo_t processo_t;
typedef struct registros_t registros_t;

typedef enum {
  EXECUCAO,
  PRONTO,
  BLOQUEADO
} estado_processo;

#endif // PROCESSO_H

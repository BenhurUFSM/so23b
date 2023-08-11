#ifndef RELOGIO_H
#define RELOGIO_H

// simulador do relógio
// registra a passagem do tempo

#include "err.h"

typedef struct relogio_t relogio_t;

// cria e inicializa um relógio
// retorna NULL em caso de erro
relogio_t *rel_cria(void);

// destrói um relógio
// nenhuma outra operação pode ser realizada no relógio após esta chamada
void rel_destroi(relogio_t *self);

// registra a passagem de uma unidade de tempo
// esta função é chamada pelo controlador após a execução de cada instrução
void rel_tictac(relogio_t *self);

// retorna a hora atual do sistema, em unidades de tempo
int rel_agora(relogio_t *self);

// Funções para acessar o relógio como um dispositivo de E/S
//   só tem leitura, e dois dispositivos, '0' para ler o relógio local
//   (contador de instruções) e '1' para ler o relógio de tempo de CPU
//   consumido pelo simulador (em ms)
err_t rel_le(void *disp, int id, int *pvalor);
#endif // RELOGIO_H

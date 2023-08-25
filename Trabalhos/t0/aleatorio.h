#ifndef ALEATORIO_H
#define ALEATORIO_H

// simulador do relógio
// registra a passagem do tempo

#include "err.h"

typedef struct aleatorio_t aleatorio_t;

// cria e inicializa um relógio
// retorna NULL em caso de erro
aleatorio_t *aleatorio_cria(void);

// destrói um relógio
// nenhuma outra operação pode ser realizada no relógio após esta chamada
void aleatorio_destroi(aleatorio_t *self);

// registra a passagem de uma unidade de tempo
// esta função é chamada pelo controlador após a execução de cada instrução
void aleatorio_gerar(aleatorio_t *self);

// retorna a hora atual do sistema, em unidades de tempo
int aleatorio_agora(aleatorio_t *self);

// Funções para acessar o aleatorioógio como um dispositivo de E/S
//   só tem leitura, e dois dispositivos, '0' para ler o aleatorioógio local
//   (contador de instruções) e '1' para ler o relógio de tempo de CPU
//   consumido pelo simulador (em ms)
err_t aleatorio_le(void *disp, int id, int *pvalor);
#endif // aleatorio_H

#ifndef MEMORIA_H
#define MEMORIA_H

// simulador da memória principal
// é um vetor de inteiros

#include "err.h"

// tipo opaco que representa a memória
typedef struct mem_t mem_t;

// cria uma região de memória com capacidade para 'tam' valores (inteiros)
// retorna um ponteiro para um descritor, que deverá ser usado em todas
//   as operações sobre essa memória
// retorna NULL em caso de erro
mem_t *mem_cria(int tam);

// destrói uma região de memória
// nenhuma outra operação pode ser realizada na região após esta chamada
void mem_destroi(mem_t *self);

// retorna o tamanho da região de memória (número de valores que comporta)
int mem_tam(mem_t *self);

// coloca na posição apontada por 'pvalor' o valor no endereço 'endereco'
// retorna erro ERR_END_INV (e não altera '*pvalor') se endereço inválido
err_t mem_le(mem_t *self, int endereco, int *pvalor);

// coloca 'valor' no endereço 'endereco' da memória
// retorna erro ERR_END_INV se endereço inválido
err_t mem_escreve(mem_t *self, int endereco, int valor);

#endif // MEMORIA_H

#ifndef PROGRAMA_H
#define PROGRAMA_H

// TAD para representar um programa lido de um arquivo '.maq'

typedef struct programa_t programa_t;

// cria e inicializa um programa com o conteúdo do arquivo 'nome'
// retorna NULL em caso de erro
programa_t *prog_cria(char *nome);

// destrói um programa
// nenhuma outra operação pode ser realizada no programa após esta chamada
void prog_destroi(programa_t *self);

// número de posições de memória necessárias para executar o programa
int prog_tamanho(programa_t *self);

// endereço inicial de carga do programa
int prog_end_carga(programa_t *self);

// endereço inicial de execução do programa
// (valor inicial do PC para executar esse programa)
int prog_end_inicio(programa_t *self);

// valor a colocar na posição 'ender' da memória
int prog_dado(programa_t *self, int ender);

#endif // PROGRAMA_H

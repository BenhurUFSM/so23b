#ifndef TABPAG_H
#define TABPAG_H

// tabela de páginas, para implementação de paginação
// estrutura auxiliar para a MMU
// realiza a tradução de endereços virtuais do espaço de endereçamento
//   de um processo em endereços físicos da memória principal

#include "err.h"
#include <stdbool.h>

// tamanho de uma página, em palavras de memória
#define TAM_PAGINA 10

// tipo opaco que representa a tabela de páginas
typedef struct tabpag_t tabpag_t;

// cria uma tabela de páginas
// retorna um ponteiro para um descritor, que deverá ser usado em todas
//   as operações nessa tabela
// retorna NULL em caso de erro
tabpag_t *tabpag_cria(void);

// destrói uma tabela de páginas
// nenhuma outra operação pode ser realizada na tabela após esta chamada
void tabpag_destroi(tabpag_t *self);

// define a tradução da página 'pagina' deve resultar no quadro 'quadro'
// se 'quadro' for -1, indica que a tradução não é possível, resultando em
//   ERR_PAG_AUSENTE
// os bits de acesso e alteração para essa página são zerados
void tabpag_define_quadro(tabpag_t *self, int pagina, int quadro);

// marca o bit de acesso à página; se alteracao for true, marca também o
//   bit de alteração
// não faz nada se a página não estiver mapeada em algum quadro
void tabpag_marca_bit_acesso(tabpag_t *self, int pagina, bool alteracao);

// zera o bit de acesso à página; não afeta o bit de alteração
// não faz nada se a página não estiver mapeada em algum quadro
void tabpag_zera_bit_acesso(tabpag_t *self, int pagina);

// retorna o valor do bit de acesso à página
// retorna false se a página não estiver mapeada em algum quadro
bool tabpag_bit_acesso(tabpag_t *self, int pagina);

// retorna o valor do bit de alteração da página
// retorna false se a página não estiver mapeada em algum quadro
bool tabpag_bit_alteracao(tabpag_t *self, int pagina);

// traduz o endereço virtual 'endvirt'; coloca o endereço físico correspondente
//   na posição apontada por 'pendfis'
// retorna erro (e não altera '*pendfis') se a tradução não for possível:
//   ERR_END_INV - página não existente na tabela de páginas
//   ERR_PAG_AUSENTE - página marcada como ausente na tabela de páginas
err_t tabpag_traduz(tabpag_t *self, int endvirt, int *pendfis);

#endif // TABPAG_H

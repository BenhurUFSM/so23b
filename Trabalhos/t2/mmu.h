#ifndef MMU_H
#define MMU_H

// simulador da unidade de gerenciamento de memória (MMU)
// realiza a tradução de endereços virtuais do espaço de endereçamento
//   de um processo em endereços físicos da memória principal
// implementa memória virtual por paginação

#include "tabpag.h"
#include "memoria.h"
#include "err.h"

// tipo opaco que representa a MMU
typedef struct mmu_t mmu_t;

// cria uma MMU para gerenciar acessos à memória
// retorna um ponteiro para um descritor, que deverá ser usado em todas
//   as operações nessa MMU
// recebe "mem", a memória física que será gerenciada
// retorna NULL em caso de erro
mmu_t *mmu_cria(mem_t *mem);

// destrói uma MMU
// nenhuma outra operação pode ser realizada na MMU após esta chamada
void mmu_destroi(mmu_t *self);

// define a tabela de páginas a usar nas próximas traduções
// se tabpag for NULL, os acessos serão repassados sem alteração à memória
void mmu_define_tabpag(mmu_t *self, tabpag_t *tabpag);

// traduz o endereço virtual 'endvirt'; coloca o endereço físico correspondente
//   na posição apontada por 'pendfis'
// retorna erro (e não altera '*pendfis') se a tradução não for possível:
//   ERR_END_INV - página não existente na tabela de páginas
//   ERR_PAG_AUSENTE - página marcada como ausente na tabela de páginas
err_t mmu_traduz(mmu_t *self, int endvirt, int *pendfis);

// coloca na posição apontada por 'pvalor' o valor que está na memória
//   no endereço físico correspondente ao endereço virtual 'endvirt'
// marca a página como acessada se o acesso for bem sucedido
// retorna erro se acesso não for possível, por um erro de tradução
//   (ver mmu_traduz) ou de memória (ver mem_le)
err_t mmu_le(mmu_t *self, int endvirt, int *pvalor);

// coloca 'valor' no endereço físico da memória correspondente ao endereço
//   virtual 'endvirt'
// marca a página como acessada e alterada se o acesso for bem sucedido
// retorna erro se acesso não for possível, por um erro de tradução
//   (ver mmu_traduz) ou de memória (ver mem_escreve)
err_t mmu_escreve(mmu_t *self, int endvirt, int valor);

#endif // MMU_H

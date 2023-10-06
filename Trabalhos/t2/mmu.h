#ifndef MMU_H
#define MMU_H

// simulador da unidade de gerenciamento de memória (MMU)
// realiza a tradução de endereços virtuais do espaço de endereçamento
//   de um processo em endereços físicos da memória principal
// implementa memória virtual por paginação

#include "tabpag.h"
#include "memoria.h"
#include "err.h"
#include "cpu_modo.h"

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

// coloca na posição apontada por 'pvalor' o valor que está na memória
//   no endereço físico correspondente ao endereço virtual 'endvirt'
// marca a página como acessada se o acesso for bem sucedido
// retorna erro se acesso não for possível, por um erro de tradução
//   (ver tabpag_traduz) ou de memória (ver mem_le)
// se o acesso for feito em modo supervisor, ou se a mmu não tiver tabela de
//   página definida, trata endvirt como enderço físico, repassa o acesso
//   à memória sem tradução
err_t mmu_le(mmu_t *self, int endvirt, int *pvalor, cpu_modo_t modo);

// coloca 'valor' no endereço físico da memória correspondente ao endereço
//   virtual 'endvirt'
// marca a página como acessada e alterada se o acesso for bem sucedido
// retorna erro se acesso não for possível, por um erro de tradução
//   (ver tabpag_traduz) ou de memória (ver mem_escreve)
// se o acesso for feito em modo supervisor, ou se a mmu não tiver tabela de
//   página definida, trata endvirt como enderço físico, repassa o acesso
//   à memória sem tradução
err_t mmu_escreve(mmu_t *self, int endvirt, int valor, cpu_modo_t modo);

#endif // MMU_H

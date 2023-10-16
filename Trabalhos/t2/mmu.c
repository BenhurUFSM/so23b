#include "mmu.h"
#include <stdlib.h>

// tipo de dados opaco para representar uma MMU
struct mmu_t {
  mem_t *mem;
  tabpag_t *tabpag;
};

mmu_t *mmu_cria(mem_t *mem)
{
  mmu_t *self;
  self = malloc(sizeof(*self));
  if (self != NULL) {
    self->mem = mem;
    self->tabpag = NULL;
  }
  return self;
}

void mmu_destroi(mmu_t *self)
{
  if (self != NULL) {
    free(self);
  }
}

void mmu_define_tabpag(mmu_t *self, tabpag_t *tabpag)
{
  self->tabpag = tabpag;
}

err_t mmu_le(mmu_t *self, int endvirt, int *pvalor, cpu_modo_t modo)
{
  if (modo == supervisor || self->tabpag == NULL) {
    return mem_le(self->mem, endvirt, pvalor);
  }
  int endfis;
  err_t err = tabpag_traduz(self->tabpag, endvirt, &endfis);
  if (err == ERR_OK) {
    err = mem_le(self->mem, endfis, pvalor);
    if (err == ERR_OK) {
      tabpag_marca_bit_acesso(self->tabpag, endvirt / TAM_PAGINA, false);
    }
  }
  return err;
}

err_t mmu_escreve(mmu_t *self, int endvirt, int valor, cpu_modo_t modo)
{
  if (modo == supervisor || self->tabpag == NULL) {
    return mem_escreve(self->mem, endvirt, valor);
  }
  int endfis;
  err_t err = tabpag_traduz(self->tabpag, endvirt, &endfis);
  if (err == ERR_OK) {
    err = mem_escreve(self->mem, endfis, valor);
    if (err == ERR_OK) {
      tabpag_marca_bit_acesso(self->tabpag, endvirt / TAM_PAGINA, true);
    }
  }
  return err;
}

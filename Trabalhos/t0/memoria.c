#include "memoria.h"
#include <stdlib.h>

// tipo de dados opaco para representar uma região de memória
struct mem_t {
  int tam;
  int *conteudo;
};

mem_t *mem_cria(int tam)
{
  mem_t *self;
  self = malloc(sizeof(*self));
  if (self != NULL) {
    self->tam = tam;
    self->conteudo = malloc(tam * sizeof(*(self->conteudo)));
    if (self->conteudo == NULL) {
      free(self);
      self = NULL;
    }
  }
  return self;
}

void mem_destroi(mem_t *self)
{
  if (self != NULL) {
    if (self->conteudo != NULL) {
      free(self->conteudo);
    }
    free(self);
  }
}

int mem_tam(mem_t *self)
{
  return self->tam;
}

// função auxiliar, verifica se endereço é válido
static err_t verif_permissao(mem_t *self, int endereco)
{
  if (endereco < 0 || endereco >= self->tam) {
    return ERR_END_INV;
  }
  return ERR_OK;
}

err_t mem_le(mem_t *self, int endereco, int *pvalor)
{
  err_t err = verif_permissao(self, endereco);
  if (err == ERR_OK) {
    *pvalor = self->conteudo[endereco];
  }
  return err;
}

err_t mem_escreve(mem_t *self, int endereco, int valor)
{
  err_t err = verif_permissao(self, endereco);
  if (err == ERR_OK) {
    self->conteudo[endereco] = valor;
  }
  return err;
}

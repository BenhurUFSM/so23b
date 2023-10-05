#include "tabpag.h"
#include "err.h"
#include <stdlib.h>
#include <assert.h>

typedef struct {
  int quadro;
  bool acessada;
  bool alterada;
} descritor_t;

struct tabpag_t {
  descritor_t *tabela;
  int tam_tab;
};

tabpag_t *tabpag_cria(void)
{
  tabpag_t *self = malloc(sizeof(*self));
  if (self == NULL) return self;
  self->tabela = NULL;
  self->tam_tab = 0;
  return self;
}

void tabpag_destroi(tabpag_t *self)
{
  if (self->tabela != NULL) free(self->tabela);
  free(self);
}

static void tabpag__remove_pagina(tabpag_t *self, int pagina)
{
  if (pagina >= self->tam_tab) return;
  if (pagina < self->tam_tab - 1) {
    self->tabela[pagina].quadro = -1;
    return;
  }
  do {
    self->tam_tab--;
  } while (self->tam_tab > 0 && self->tabela[self->tam_tab - 1].quadro == -1);
  if (self->tam_tab == 0) {
    free(self->tabela);
    self->tabela = NULL;
  } else {
    self->tabela = realloc(self->tabela, self->tam_tab * sizeof(descritor_t));
    assert(self->tabela != NULL);
  }
}

static void tabpag__insere_pagina(tabpag_t *self, int pagina)
{
  if (pagina < self->tam_tab) return;
  int novo_tam = pagina + 1;
  if (self->tam_tab == 0) {
    self->tabela = malloc(novo_tam * sizeof(descritor_t));
  } else {
    self->tabela = realloc(self->tabela, novo_tam * sizeof(descritor_t));
  }
  assert(self->tabela != NULL);
  while (self->tam_tab < novo_tam) {
    self->tabela[self->tam_tab].quadro = -1;
    self->tam_tab++;
  }
}

void tabpag_define_quadro(tabpag_t *self, int pagina, int quadro)
{
  if (quadro == -1) {
    tabpag__remove_pagina(self, pagina);
  } else {
    tabpag__insere_pagina(self, pagina);
    self->tabela[pagina].quadro = quadro;
    self->tabela[pagina].acessada = false;
    self->tabela[pagina].alterada = false;
  }
}

void tabpag_marca_bit_acesso(tabpag_t *self, int pagina, bool alteracao)
{
  if (pagina < self->tam_tab) {
    self->tabela[pagina].acessada = true;
    if (alteracao) {
      self->tabela[pagina].alterada = true;
    }
  }
}

void tabpag_zera_bit_acesso(tabpag_t *self, int pagina)
{
  if (pagina < self->tam_tab) {
    self->tabela[pagina].acessada = false;
  }
}

bool tabpag_bit_acesso(tabpag_t *self, int pagina)
{
  if (pagina < self->tam_tab) {
    return self->tabela[pagina].acessada;
  }
  return false;
}

bool tabpag_bit_alteracao(tabpag_t *self, int pagina)
{
  if (pagina < self->tam_tab) {
    return self->tabela[pagina].alterada;
  }
  return false;
}

err_t tabpag_traduz(tabpag_t *self, int endvirt, int *pendfis)
{
  int pagina = endvirt / TAM_PAGINA;
  if (pagina >= self->tam_tab) return ERR_END_INV;
  int quadro = self->tabela[pagina].quadro;
  if (quadro == -1) return ERR_PAG_AUSENTE;
  int deslocamento = endvirt % TAM_PAGINA;
  *pendfis = quadro * TAM_PAGINA + deslocamento;
  return ERR_OK;
}

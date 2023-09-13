#include "relogio.h"
#include <stdlib.h>
#include <time.h>

struct relogio_t {
  int agora;             // que horas são
  int t_ate_interrupcao; // quanto tempo até gerar uma interrupcao
  int interrupcao;       // 1 se está gerando interrupcao, 0 se não
};

relogio_t *rel_cria(void)
{
  relogio_t *self;
  self = malloc(sizeof(relogio_t));
  if (self != NULL) {
    self->agora = 0;
    self->t_ate_interrupcao = 0;
    self->interrupcao = 0;
  }
  return self;
}

void rel_destroi(relogio_t *self)
{
  free(self);
}

void rel_tictac(relogio_t *self)
{
  self->agora++;
  // vê se tem que gerar interrupção
  if (self->t_ate_interrupcao != 0) {
    self->t_ate_interrupcao--;
    if (self->t_ate_interrupcao == 0) {
      self->interrupcao = 1;
    }
  }
}

int rel_agora(relogio_t *self)
{
  return self->agora;
}

err_t rel_le(void *disp, int id, int *pvalor)
{
  relogio_t *self = disp;
  err_t err = ERR_OK;
  switch (id) {
    case 0:
      *pvalor = self->agora;
      break;
    case 1:
      *pvalor = clock()/(CLOCKS_PER_SEC/1000);
      break;
    case 2:
      *pvalor = self->t_ate_interrupcao;
      break;
    case 3:
      *pvalor = self->interrupcao;
      break;
    default: 
      err = ERR_END_INV;
  }
  return err;
}

err_t rel_escr(void *disp, int id, int pvalor)
{
  relogio_t *self = disp;
  err_t err = ERR_OK;
  switch (id) {
    case 2:
      self->t_ate_interrupcao = pvalor;
      break;
    case 3:
      self->interrupcao = (pvalor == 0) ? 0 : 1;
      break;
    default: 
      err = ERR_END_INV;
  }
  return err;
}

#include "relogio.h"
#include <stdlib.h>
#include <time.h>

struct relogio_t {
  int agora;
};

relogio_t *rel_cria(void)
{
  relogio_t *self;
  self = malloc(sizeof(relogio_t));
  if (self != NULL) {
    self->agora = 0;
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
    default: 
      err = ERR_END_INV;
  }
  return err;
}

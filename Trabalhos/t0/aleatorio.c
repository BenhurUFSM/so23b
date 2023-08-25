#include "aleatorio.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

struct aleatorio_t {
  int numero;
};

aleatorio_t *aleatorio_cria(void)
{
  aleatorio_t *self;
  self = malloc(sizeof(aleatorio_t));
  if (self != NULL) {
    self->numero = 0;
  }
  return self;
}

void aleatorio_destroi(aleatorio_t *self)
{
  free(self);
}

void aleatorio_gerar(aleatorio_t *self)
{
  self->numero = (rand() % 100)+1;
}

int aleatorio_numero(aleatorio_t *self)
{
  return self->numero;
}

err_t aleatorio_le(void *disp, int id, int *pvalor)
{
  aleatorio_t *self = disp;
  err_t err = ERR_OK;
 
  switch (id) { 
    case 0:
      *pvalor = self->numero;
      break;
    case 1:
      aleatorio_gerar(disp);
      *pvalor = self->numero;
      //  printf("id %d", self->numero); //pode ser descomentado para saber o número e testar
      break;
    default: 
      err = ERR_END_INV;
  }
  return err;
}

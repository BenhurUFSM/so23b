#include "es.h"
#include <stdio.h>
#include <stdlib.h>

// estrutura para definir um dispositivo
typedef struct {
   f_le_t f_le;         // função para ler um inteiro do dispositivo
   f_escr_t f_escr;     // função para escrever um inteiro no dispositivo
   void *controladora;  // descritor do dispositivo (arg das f acima)
   int id;              // identificador do (sub)dispositivo (arg das f acima)
} dispositivo_t;

#define N_DISPO 100 // número máximo de dispositivos suportados

// define a estrutura opaca
struct es_t {
  dispositivo_t dispositivos[N_DISPO];
};

es_t *es_cria(void)
{
  es_t *self = calloc(1, sizeof(*self)); // com calloc já zera toda a struct
  return self;
}

void es_destroi(es_t *self)
{
  free(self);
}

bool es_registra_dispositivo(es_t *self, int dispositivo,
                             void *controladora, int id,
                             f_le_t f_le, f_escr_t f_escr)
{
  if (dispositivo < 0 || dispositivo >= N_DISPO) return false;
  self->dispositivos[dispositivo].controladora = controladora;
  self->dispositivos[dispositivo].id = id;
  self->dispositivos[dispositivo].f_le = f_le;
  self->dispositivos[dispositivo].f_escr = f_escr;
  return true;
}

err_t es_le(es_t *self, int dispositivo, int *pvalor)
{
  if (dispositivo < 0 || dispositivo >= N_DISPO) return ERR_DISP_INV;
  if (self->dispositivos[dispositivo].f_le == NULL) return ERR_OP_INV;
  void *controladora = self->dispositivos[dispositivo].controladora;
  int id = self->dispositivos[dispositivo].id;
  return self->dispositivos[dispositivo].f_le(controladora, id, pvalor);
}

err_t es_escreve(es_t *self, int dispositivo, int valor)
{
  if (dispositivo < 0 || dispositivo >= N_DISPO) return ERR_DISP_INV;
  if (self->dispositivos[dispositivo].f_escr == NULL) return ERR_OP_INV;
  void *controladora = self->dispositivos[dispositivo].controladora;
  int id = self->dispositivos[dispositivo].id;
  return self->dispositivos[dispositivo].f_escr(controladora, id, valor);
}

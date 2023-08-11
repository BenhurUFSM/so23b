#include "controle.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct controle_t {
  cpu_t *cpu;
  relogio_t *relogio;
  console_t *console;
  enum { executando, passo, parado, fim } estado;
};

// funções auxiliares
static void controle_processa_teclado(controle_t *self);
static void controle_atualiza_console(controle_t *self);


controle_t *controle_cria(cpu_t *cpu, console_t *console, relogio_t *relogio)
{
  controle_t *self = malloc(sizeof(*self));
  if (self == NULL) return NULL;

  self->cpu = cpu;
  self->console = console;
  self->relogio = relogio;
  self->estado = parado;

  return self;
}

void controle_destroi(controle_t *self)
{
  free(self);
}

void controle_laco(controle_t *self)
{
  // executa uma instrução por vez até a console dizer que chega
  do {
    if (self->estado == passo || self->estado == executando) {
      err_t err;
      err = cpu_executa_1(self->cpu);
      if (err != ERR_OK) {
        self->estado = parado;
      }
      rel_tictac(self->relogio);
    }
    controle_processa_teclado(self);
    controle_atualiza_console(self);
  } while (self->estado != fim);

  console_printf(self->console, "Fim da execução.");
  console_printf(self->console, "relógio: %d\n", rel_agora(self->relogio));
}
 

static void controle_processa_teclado(controle_t *self)
{
  if (self->estado == passo) self->estado = parado;
  char cmd = console_processa_entrada(self->console);
  switch (cmd) {
    case 'F':
      self->estado = fim;
      break;
    case 'P':
      self->estado = parado;
      break;
    case '1':
      self->estado = passo;
      break;
    case 'C':
      self->estado = executando;
      break;
  }
}

static void controle_atualiza_console(controle_t *self)
{
  char *status = cpu_descricao(self->cpu);
  console_print_status(self->console, status);
  console_atualiza(self->console);
}

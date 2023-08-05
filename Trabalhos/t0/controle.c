#include "controle.h"
#include "memoria.h"
#include "cpu.h"
#include "relogio.h"
#include "console.h"
#include "es.h"
#include "instrucao.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>



struct controle_t {
  mem_t *mem;
  cpu_t *cpu;
  relogio_t *relogio;
  console_t *console;
  es_t *es;
  enum { executando, passo, parado, fim } estado;
};

// funções auxiliares
static void controle_processa_teclado(controle_t *self);
static void controle_atualiza_console(controle_t *self);


controle_t *controle_cria(void)
{
  controle_t *self = malloc(sizeof(*self));
  if (self == NULL) return NULL;

  // cria a memória
  self->mem = mem_cria(MEM_TAM);

  // cria dispositivos de E/S
  self->console = console_cria();
  self->relogio = rel_cria();

  // cria o controlador de E/S e registra os dispositivos
  self->es = es_cria();
  // lê teclado, testa teclado, escreve tela, testa tela do terminal A
  es_registra_dispositivo(self->es, 0, self->console, 0, term_le, NULL);
  es_registra_dispositivo(self->es, 1, self->console, 1, term_le, NULL);
  es_registra_dispositivo(self->es, 2, self->console, 2, NULL, term_escr);
  es_registra_dispositivo(self->es, 3, self->console, 3, term_le, NULL);
  // lê teclado, testa teclado, escreve tela, testa tela do terminal B
  es_registra_dispositivo(self->es, 4, self->console, 4, term_le, NULL);
  es_registra_dispositivo(self->es, 5, self->console, 5, term_le, NULL);
  es_registra_dispositivo(self->es, 6, self->console, 6, NULL, term_escr);
  es_registra_dispositivo(self->es, 7, self->console, 7, term_le, NULL);
  // lê relógio virtual, relógio real
  es_registra_dispositivo(self->es, 8, self->relogio, 0, rel_le, NULL);
  es_registra_dispositivo(self->es, 9, self->relogio, 1, rel_le, NULL);

  // cria a unidade de execução e inicializa com a memória e E/S
  self->cpu = cpu_cria(self->mem, self->es);

  self->estado = parado;

  return self;
}

void controle_destroi(controle_t *self)
{
  // destroi todo mundo!
  cpu_destroi(self->cpu);
  es_destroi(self->es);
  console_destroi(self->console);
  rel_destroi(self->relogio);
  mem_destroi(self->mem);
  free(self);
}

mem_t *controle_mem(controle_t *self)
{
  return self->mem;
}

cpu_t *controle_cpu(controle_t *self)
{
  return self->cpu;
}

es_t *controle_es(controle_t *self)
{
  return self->es;
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
  char *s = cpu_descricao(self->cpu);
  console_print_status(self->console, s);
  console_atualiza(self->console);
}

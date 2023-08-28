#include "so.h"
#include "irq.h"
#include "programa.h"

#include <stdlib.h>
#include <stdbool.h>

struct so_t {
  cpu_t *cpu;
  mem_t *mem;
  console_t *console;
};


// funções auxiliares
static bool so_carrega_programa(so_t *self, char *nome_do_executavel);



// função a ser chamada pela CPU quando executa a instrução CHAMAC
// essa instrução só deve ser executada quando for tratar uma interrupção
// o primeiro argumento é um ponteiro para o SO, o segundo é a identificação
//   da interrupção
// na inicialização do SO é colocada no endereço 10 uma rotina que executa
//   CHAMAC; quando recebe uma interrupção, a CPU salva os registradores
//   no endereço 0, e desvia para o endereço 10
static int so_trata_interrupcao(void *argC, int reg_A)
{
  so_t *self = argC;
  irq_t irq = reg_A;
  console_printf(self->console, "SO: recebi IRQ %d", irq);
  if (irq == IRQ_RESET) {
    // coloca um programa na memória
    so_carrega_programa(self, "ex1.maq");
    // altera o PC para o endereço de carga (deve ter sido 100)
    mem_escreve(self->mem, IRQ_END_PC, 100);
    // passa o processador para modo usuário
    mem_escreve(self->mem, IRQ_END_modo, usuario);
  }
  return 0;
}

so_t *so_cria(cpu_t *cpu, mem_t *mem, console_t *console)
{
  so_t *self = malloc(sizeof(*self));
  if (self == NULL) return NULL;

  self->cpu = cpu;
  self->mem = mem;
  self->console = console;

  // quando a CPU executar uma instrução CHAMAC, deve chamar essa função
  cpu_define_chamaC(self->cpu, so_trata_interrupcao, self);
  // coloca o tratador de interrupção na memória
  so_carrega_programa(self, "trata_irq.maq");

  return self;
}

void so_destroi(so_t *self)
{
  cpu_define_chamaC(self->cpu, NULL, NULL);
  free(self);
}

// carrega o programa na memória
static bool so_carrega_programa(so_t *self, char *nome_do_executavel)
{
  // programa para executar na nossa CPU
  programa_t *prog = prog_cria(nome_do_executavel);
  if (prog == NULL) {
    console_printf(self->console,
        "Erro na leitura do programa '%s'\n", nome_do_executavel);
    return false;
  }

  int end_ini = prog_end_carga(prog);
  int end_fim = end_ini + prog_tamanho(prog);

  for (int end = end_ini; end < end_fim; end++) {
    if (mem_escreve(self->mem, end, prog_dado(prog, end)) != ERR_OK) {
      console_printf(self->console,
          "Erro na carga da memória, endereco %d\n", end);
      return false;
    }
  }
  prog_destroi(prog);
  return true;
}

#include "irq.h"

static char *nomes[N_IRQ] = {
  [IRQ_RESET] =   "Reset",
  [IRQ_ERR_CPU] = "Erro de execução",
  [IRQ_SISTEMA] = "Chamada de sistema",
  [IRQ_RELOGIO] = "E/S: relógio",
  [IRQ_TECLADO] = "E/S: teclado",
  [IRQ_TELA]    = "E/S: console",
};

// retorna o nome da interrupção
char *irq_nome(irq_t irq)
{
  if (irq < 0 || irq >= N_IRQ) return "DESCONHECIDA";
  return nomes[irq];
}

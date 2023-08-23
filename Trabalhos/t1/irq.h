#ifndef IRQ_H
#define IRQ_H

// os motivos para uma requisição de interrupção (irq) ser gerada
typedef enum {
  IRQ_ERR_CPU,       // erro interno na CPU (ver registrador de erro)
  IRQ_SISTEMA,       // chamada de sistema
  IRQ_RELOGIO,       // interrupção causada pelo relógio
  IRQ_TECLADO,       // interrupção causada pelo teclado
  IRQ_TELA,          // interrupção causada pela tela
  N_IRQ              // número de interrupções
} irq_t;

#endif // IRQ_H

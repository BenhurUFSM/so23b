#ifndef CONSOLE_H
#define CONSOLE_H

// entrada e saída de caracteres, simulando vários terminais independentes
//
// a entrada pode ser feita programaticamente (com a função t_ins) ou
//   interativamente, digitando uma letra (a para o primeiro terminal, 
//   b para o segundo etc) e os caracteres a entrar (se nenhum caractere for
//   colocado após a letra, será inserido um enter)
//
// além da saída em cada terminal, tem a saída da console, com t_printf (para debug)

#include <stdbool.h>
#include "es.h"

typedef struct console_t console_t;

// cria e inicializa a console
// retorna NULL em caso de erro
console_t *console_cria(void);

// destrói a console
void console_destroi(console_t *self);

// imprime na área geral do console
int console_printf(console_t *self, char *fmt, ...);

// imprime na linha de status
void console_print_status(console_t *self, char *txt);

// função chamada para receber comandos externos da console
// retorna um char que representa um comando do usuário que deve ser
//   tratado fora da console (atualmente 'P', '1' ou 'C' - para, executa 1 instrução
//   ou continua a execução)
char console_processa_entrada(console_t *self);

// esta função deve ser chamada periodicamente para que tela funcione
void console_tictac(console_t *self);

// esta função deve ser chamada para desenhar a tela da console
void console_atualiza(console_t *self);

// Funções para implementar o protocolo de acesso a um dispositivo pelo
//   controlador de E/S
err_t term_le(void *disp, int id, int *pvalor);
err_t term_escr(void *disp, int id, int valor);

int obtem_proximo_terminal_disponivel(console_t *self);
int obtem_id_por_tid(int tid, int op);

#endif // CONSOLE_H

#include "console.h"

#include <string.h>
#include <curses.h>  // tomara que eu não me arrependa!
#include <locale.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>


// tamanho da tela -- a janela do terminal tem que ter pelo menos esse tamanho
#define N_LIN 24  // número de linhas na tela
#define N_COL 80  // número de colunas na tela

// número de linhas para cada componente da tela
#define N_TERM        4  // número de terminais, cada um ocupa 2 linhas na tela
#define N_LIN_TERM    (N_TERM * 2)
#define N_LIN_STATUS  1
#define N_LIN_ENTRADA 1
#define N_LIN_CONSOLE (N_LIN - N_LIN_TERM - N_LIN_STATUS - N_LIN_ENTRADA)

// linha onde começa cada componente
#define LINHA_TERM    0
#define LINHA_STATUS  (LINHA_TERM + N_LIN_TERM)
#define LINHA_CONSOLE (LINHA_STATUS + N_LIN_STATUS)
#define LINHA_ENTRADA (LINHA_CONSOLE + N_LIN_CONSOLE)

// identificação da cor para cada parte
#define COR_TXT_PAR      1
#define COR_CURSOR_PAR   2
#define COR_TXT_IMPAR    3
#define COR_CURSOR_IMPAR 4
#define COR_CONSOLE      5
#define COR_ENTRADA      6
#define COR_OCUPADO      7

// dados para cada terminal
typedef struct {
  // texto já digitado no terminal, esperando para ser lido
  char entrada[N_COL+1];
  // texto sendo mostrado na saída do terminal
  char saida[N_COL+1];
  // normal: aceitando novos caracteres na saída
  // rolando: removendo um caractere no início para gerar espaço
  //   o \0 tem a posição do rolamento, move um caractere por vez para
  //   antes do \0, até chegar no \0 do fim, quando volta a 'normal'.
  //   entra neste estado quando recebe um caractere na última posição.
  //   não aceita novos caracteres
  // limpando: removendo um caractere no início da linha por vez, até
  //   ficar com a linha vazia.
  //   entra nesse estado quando recebe um '\n'.
  //   não aceita novos caracteres
  enum { normal, rolando, limpando } estado_saida;
  int cor_txt;
  int cor_cursor;
} term_t;

struct console_t {
  term_t term[N_TERM];
  char txt_status[N_COL+1];
  char txt_console[N_LIN_CONSOLE][N_COL+1];
  char digitando[N_COL+1];
};

// funções auxiliares
static void init_curses(void);

console_t *console_cria(void)
{
  console_t *self = malloc(sizeof(*self));
  if (self == NULL) return NULL;

  for (int t=0; t<N_TERM; t++) {
    self->term[t].entrada[0] = '\0';
    self->term[t].saida[0] = '\0';
    self->term[t].estado_saida = normal;
    if (t%2 == 0) {
      self->term[t].cor_txt = COR_TXT_PAR;
      self->term[t].cor_cursor = COR_CURSOR_PAR;
    } else {
      self->term[t].cor_txt = COR_TXT_IMPAR;
      self->term[t].cor_cursor = COR_CURSOR_IMPAR;
    }
  }
  for (int l=0; l<N_LIN_CONSOLE; l++) {
    self->txt_console[l][0] = '\0';
  }
  self->digitando[0] = '\0';

  init_curses();

  return self;
}

// inicializa o curses
static void init_curses(void)
{
  setlocale(LC_ALL, "");  // para ter suporte a UTF8
  initscr();
  cbreak();      // lê cada char, não espera enter
  noecho();      // não mostra o que é digitado
  timeout(5);    // não espera digitar, retorna ERR se nada foi digitado
  start_color();
  init_pair(COR_TXT_PAR, COLOR_GREEN, COLOR_BLACK);
  init_pair(COR_CURSOR_PAR, COLOR_BLACK, COLOR_GREEN);
  init_pair(COR_TXT_IMPAR, COLOR_YELLOW, COLOR_BLACK);
  init_pair(COR_CURSOR_IMPAR, COLOR_BLACK, COLOR_YELLOW);
  init_pair(COR_CONSOLE, COLOR_BLUE, COLOR_BLACK);
  init_pair(COR_ENTRADA, COLOR_GREEN, COLOR_BLACK);
  init_pair(COR_OCUPADO, COLOR_BLACK, COLOR_RED);
}

void console_destroi(console_t *self)
{
  console_atualiza(self);
  attron(COLOR_PAIR(COR_OCUPADO));
  addstr("  digite ENTER para sair  ");
  while (getch() != '\n') {
    ;
  }
  // acaba com o curses
  endwin();

  free(self);
  return;
}


// SAIDA

static bool pode_imprimir_no_term(console_t *self, int t)
{
  return self->term[t].estado_saida == normal;
}

static void imprime_no_term(console_t *self, int t, char ch)
{
  if (pode_imprimir_no_term(self, t)) {
    if (ch == '\n') {
      self->term[t].estado_saida = limpando;
      return;
    }
    int tam = strlen(self->term[t].saida);
    self->term[t].saida[tam] = ch;
    tam++;
    self->term[t].saida[tam] = '\0';
    if (tam >= N_COL - 1) {
      self->term[t].estado_saida = rolando;
      self->term[t].saida[0] = '\0';
    }
  }
}

static void rola_saida(term_t *termp)
{
  char *p = termp->saida;
  int tam = strlen(p);
  p[tam] = p[tam+1];
  if (p[tam] == '\0') {
    termp->estado_saida = normal;
  } else {
    p[tam+1] = '\0';
  }
}

static void limpa_saida(term_t *termp)
{
  char *p = termp->saida;
  int tam = strlen(p);
  memmove(p, p+1, tam);
  if (tam <= 1) {
    termp->estado_saida = normal;
  }
}

static void rola_saidas(console_t *self)
{
  for (int t = 0; t < N_TERM; t++) {
    term_t *termp = &self->term[t];
    switch (termp->estado_saida) {
      case normal: 
        break;
      case rolando:
        rola_saida(termp);
        break;
      case limpando:
        limpa_saida(termp);
        break;
    }
  }
}


// ENTRADA

static bool tem_char_no_term(console_t *self, int t)
{
  return self->term[t].entrada[0] != '\0';
}

static char remove_char_do_term(console_t *self, int t)
{
  if (!tem_char_no_term(self, t)) return 0;
  char *p = self->term[t].entrada;
  char ch = *p;
  memmove(p, p+1, strlen(p));
  return ch;
}

static void insere_char_no_term(console_t *self, int t, char ch)
{
  char *p = self->term[t].entrada;
  int tam = strlen(p);
  if (tam >= N_COL-2) return;
  p[tam] = ch;
  p[tam+1] = '\0';
}


// CONSOLE

static void insere_string_na_console(console_t *self, char *s)
{
  for(int l=0; l<N_LIN_CONSOLE-1; l++) {
    strncpy(self->txt_console[l], self->txt_console[l+1], N_COL);
    self->txt_console[l][N_COL] = '\0'; // quem definiu strncpy é estúpido!
  }
  strncpy(self->txt_console[N_LIN_CONSOLE-1], s, N_COL);
  self->txt_console[N_LIN_CONSOLE-1][N_COL] = '\0'; // grrrr
}

static void insere_strings_na_console(console_t *self, char *s)
{
  while (*s != '\0') {
    char *f = strchr(s, '\n');
    if (f != NULL) {
      *f = '\0';
    }
    insere_string_na_console(self, s);
    if (f == NULL) break;
    s = f+1;
  }
}

void console_print_status(console_t *self, char *txt)
{
  // imprime alinhado a esquerda ("-"), max N_COL chars ("*")
  sprintf(self->txt_status, "%-*s", N_COL, txt);
}

int console_printf(console_t *self, char *formato, ...)
{
  // esta função usa número variável de argumentos. Dá uma olhada em:
  // https://www.geeksforgeeks.org/variadic-functions-in-c/
  char s[sizeof(self->txt_console)];
  va_list arg;
  va_start(arg, formato);
  int r = vsnprintf(s, sizeof(s), formato, arg);
  insere_strings_na_console(self, s);
  return r;
}

// COMANDO

// retorna o terminal correspondente ao caractere dado, ou -1
static int term(char c)
{
  int t = tolower(c) - 'a';
  if (t >= 0 && t < N_TERM) return t;
  return -1;
}

static void insere_str_no_term(console_t *self, char c, char *str)
{
  // insere caracteres no terminal (e espaço no final)
  int t = term(c);
  if (t == -1) {
    console_printf(self, "Terminal '%c' inválido\n", c);
    return;
  }
  char *p = str;
  while (*p != '\0') {
    insere_char_no_term(self, t, *p);
    p++;
  }
  insere_char_no_term(self, t, ' ');
}

static void limpa_saida_do_term(console_t *self, char c)
{
  int t = term(c);
  if (t == -1) {
    console_printf(self, "Terminal '%c' inválido\n", c);
    return;
  }
  self->term[t].saida[0] = '\0';
  self->term[t].estado_saida = normal;
}

static char interpreta_entrada(console_t *self, char *comando)
{
  // interpreta uma linha digitada pelo operador
  // Comandos aceitos:
  // Etstr entra a string 'str' no terminal 't'  ex: eb30
  // Zt    esvazia a saída do terminal 't'  ex: za
  // P     para a execução
  // 1     executa uma instrução
  // C     continua a execução
  // F     fim da simulação
  // retorna o caractere correspondente ao comando se não tratar localmente
  //   (comandos de controle da execução), ou '\0'
  console_printf(self, "%s", comando);
  char ret = '\0';
  char cmd = toupper(comando[0]);
  switch (cmd) {
    case 'E':
      insere_str_no_term(self, comando[1], &comando[2]);
      break;
    case 'Z':
      limpa_saida_do_term(self, comando[1]);
      break;
    case 'P':
    case '1':
    case 'C':
    case 'F':
      ret = cmd;
      break;
    default:
      console_printf(self, "Comando '%c' não reconhecido", cmd);
  }
  self->digitando[0] = '\0';
  return ret;
}

// lê e guarda um caractere do teclado; retorna true se for 'enter'
static bool verifica_entrada(console_t *self)
{
  int ch = getch();
  if (ch == ERR) return false;
  int l = strlen(self->digitando);
  if (ch == '\b' || ch == 127) {   // backspace ou del
    if (l > 0) {
      self->digitando[l-1] = '\0';
    }
  } else if (ch == '\n') {
    return true;
  } else if (ch >= ' ' && ch < 127 && l < N_COL) {
    self->digitando[l] = ch;
    self->digitando[l+1] = '\0';
  } // senão, ignora o caractere digitado
  return false;
}


// DESENHO

static void desenha_terminal(term_t *termp, int linha)
{
  mvprintw(linha, 0, "%-*s", N_COL, "");
  mvprintw(linha, 0, "%s", termp->saida);
  attron(COLOR_PAIR(termp->cor_cursor));
  printw(" ");
  attroff(COLOR_PAIR(termp->cor_cursor));
  if (termp->estado_saida != normal) {
    printw("%s", termp->saida + strlen(termp->saida) + 1);
  }

  mvprintw(linha + 1, 0, "%-*s", N_COL, "");
  mvprintw(linha + 1, 0, "%s", termp->entrada);
  attron(COLOR_PAIR(termp->cor_cursor));
  printw(" ");
  attroff(COLOR_PAIR(termp->cor_cursor));
}

static void desenha_terminais(console_t *self)
{
  for (int t=0; t<N_TERM; t++) {
    term_t *termp = &self->term[t];
    int linha = LINHA_TERM + t*2;
    attron(COLOR_PAIR(termp->cor_txt));
    desenha_terminal(termp, linha);
    attroff(COLOR_PAIR(termp->cor_txt));
  }
}

static void desenha_status(console_t *self)
{
  attron(COLOR_PAIR(4));
  mvprintw(LINHA_STATUS, 0, "%-*s", N_COL, self->txt_status);
  attroff(COLOR_PAIR(4));
}

static void desenha_console(console_t *self)
{
  attron(COLOR_PAIR(COR_CONSOLE));
  for (int l=0; l<N_LIN_CONSOLE; l++) {
    int y = LINHA_CONSOLE + l;
    mvprintw(y, 0, "%-*s", N_COL, self->txt_console[l]);
  }
  attroff(COLOR_PAIR(COR_CONSOLE));
}

static void desenha_entrada(console_t *self)
{
  attron(COLOR_PAIR(COR_ENTRADA));
  mvprintw(LINHA_ENTRADA, 0, "%*s", N_COL, 
           "P=para C=continua 1=passo F=fim  Ets=entra Zt=zera");
  mvprintw(LINHA_ENTRADA, 0, "%s", self->digitando);
  attroff(COLOR_PAIR(COR_ENTRADA));
}

char console_processa_entrada(console_t *self)
{
  if (verifica_entrada(self)) {
    return interpreta_entrada(self, self->digitando);
  }
  return '\0';
}

void console_atualiza(console_t *self)
{
  rola_saidas(self);

  desenha_terminais(self);
  desenha_status(self);
  desenha_console(self);
  desenha_entrada(self);

  // manda o curses fazer aparecer tudo isso
  refresh();
}


err_t term_le(void *disp, int id, int *pvalor)
{
  console_t *self = disp;
  // cada terminal tem 4 dispositivos:
  //   leitura, estado da leitura, escrita, estado da escrita
  int term = id / 4;
  int sub = id % 4;
  if (term < 0 || term >= N_TERM) return ERR_DISP_INV;
  switch (sub) {
    case 0: // leitura do teclado
      if (!tem_char_no_term(self, term)) return ERR_OCUP;
      *pvalor = remove_char_do_term(self, term);
      break;
    case 1: // estado do teclado
      if (tem_char_no_term(self, term)) {
        *pvalor = 1;
      } else {
        *pvalor = 0;
      }
      break;
    case 2: // escrita na tela
      return ERR_OP_INV;
      break;
    case 3: // estado da tela
      if (pode_imprimir_no_term(self, term)) {
        *pvalor = 1;
      } else {
        *pvalor = 0;
      }
      break;
  }
  return ERR_OK;
}

err_t term_escr(void *disp, int id, int valor)
{
  console_t *self = disp;
  // cada terminal tem 4 dispositivos:
  //   leitura, estado da leitura, escrita, estado da escrita
  int term = id / 4;
  int sub = id % 4;
  if (term < 0 || term >= N_TERM) return ERR_DISP_INV;
  switch (sub) {
    case 0: // leitura do teclado
      return ERR_OP_INV;
      break;
    case 1: // estado do teclado
      return ERR_OP_INV;
      break;
    case 2: // escrita na tela
      if (!pode_imprimir_no_term(self, term)) return ERR_OCUP;
      imprime_no_term(self, term, valor);
      break;
    case 3: // estado da tela
      return ERR_OP_INV;
      break;
  }
  return ERR_OK;
}

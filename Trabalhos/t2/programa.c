#include "programa.h"
#include <stdio.h>
#include <stdlib.h>

struct programa_t {
  int carga;
  int tamanho;
  int *dados;
};

// lê os dados do cabeçalho do arquivo (1ª linha)
// tem "MAQ" seguido do tamanho e endereço inicial do programa
static programa_t *pega_cabecalho(char *lin)
{
  int tam, carga;
  if (sscanf(lin, "MAQ %d %d", &tam, &carga) != 2) return NULL;
  programa_t *prog = malloc(sizeof(*prog));
  if (prog == NULL) return NULL;
  prog->dados = calloc(sizeof(int), tam);
  if (prog->dados == NULL) {
    free(prog);
    return NULL;
  }
  prog->tamanho = tam;
  prog->carga = carga;
  return prog;
}

// lê os dados de uma linha
// a linha tem o endereço inicial dos seus dados entre colchetes,
// seguido dos dados, cada um seguido por vírgula
static void pega_dados(programa_t *self, char *lin)
{
  int ender;
  int pos, p;
  if (sscanf(lin, " [%d] =%n", &ender, &pos) != 1) return;
  ender -= self->carga;
  int dado;
  while (sscanf(lin+pos, "%d ,%n", &dado, &p) == 1) {
    if (ender < 0 || ender >= self->tamanho) break;
    self->dados[ender] = dado;
    ender++;
    pos += p;
  }
}

programa_t *prog_cria(char *nome)
{
  FILE *arq = fopen(nome, "r");
  if (arq == NULL) return NULL;
  char *linha = NULL;
  size_t tam_lin;
  programa_t *prog = NULL;
  if (getline(&linha, &tam_lin, arq) == -1) goto fim;
  prog = pega_cabecalho(linha);
  if (prog == NULL) goto fim;
  while (getline(&linha, &tam_lin, arq) != -1) {
    pega_dados(prog, linha);
  }
fim:
  free(linha);
  fclose(arq);
  return prog;
}

void prog_destroi(programa_t *self)
{
  free(self->dados);
  free(self);
}

int prog_tamanho(programa_t *self)
{
  return self->tamanho;
}

int prog_end_carga(programa_t *self)
{
  return self->carga;
}

int prog_end_inicio(programa_t *self)
{
  return self->carga;
}

int prog_dado(programa_t *self, int ender)
{
  if (ender < self->carga || ender >= self->carga + self->tamanho) return -1;
  return self->dados[ender - self->carga];
}

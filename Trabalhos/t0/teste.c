#include "controle.h"
#include "programa.h"
#include "memoria.h"

#include <stdio.h>
#include <stdlib.h>

// funções auxiliares
static void init_mem(mem_t *mem, char *nome_do_executavel);

int main()
{
  // cria o hardware
  controle_t *hardware = controle_cria();
  init_mem(controle_mem(hardware), "ex6.maq");
  
  controle_laco(hardware);
      
  controle_destroi(hardware);
  return 0;
}


// cria memória e inicializa com o conteúdo do programa
static void init_mem(mem_t *mem, char *nome_do_executavel)
{
  // programa para executar na nossa CPU
  programa_t *prog = prog_cria(nome_do_executavel);
  if (prog == NULL) {
    fprintf(stderr, "Erro na leitura do programa '%s'\n", nome_do_executavel);
    exit(1);
  }

  int end_ini = prog_end_carga(prog);
  int end_fim = end_ini + prog_tamanho(prog);

  for (int end = end_ini; end < end_fim; end++) {
    if (mem_escreve(mem, end, prog_dado(prog, end)) != ERR_OK) {
      printf("Erro na carga da memória, endereco %d\n", end);
      exit(1);
    }
  }
  prog_destroi(prog);
}

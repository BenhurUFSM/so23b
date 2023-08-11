#include "controle.h"
#include "programa.h"
#include "memoria.h"
#include "cpu.h"
#include "relogio.h"
#include "console.h"

#include <stdio.h>
#include <stdlib.h>

// constantes
#define MEM_TAM 2000        // tamanho da memória principal


// funções auxiliares
static void init_mem(mem_t *mem, char *nome_do_executavel);

static struct {
  mem_t *mem;
  cpu_t *cpu;
  relogio_t *relogio;
  console_t *console;
  es_t *es;
  controle_t *controle;
} hardware;

void cria_hardware()
{

  // cria a memória
  hardware.mem = mem_cria(MEM_TAM);

  // cria dispositivos de E/S
  hardware.console = console_cria();
  hardware.relogio = rel_cria();

  // cria o controlador de E/S e registra os dispositivos
  hardware.es = es_cria();
  // lê teclado, testa teclado, escreve tela, testa tela do terminal A
  es_registra_dispositivo(hardware.es, 0, hardware.console, 0, term_le, NULL);
  es_registra_dispositivo(hardware.es, 1, hardware.console, 1, term_le, NULL);
  es_registra_dispositivo(hardware.es, 2, hardware.console, 2, NULL, term_escr);
  es_registra_dispositivo(hardware.es, 3, hardware.console, 3, term_le, NULL);
  // lê teclado, testa teclado, escreve tela, testa tela do terminal B
  es_registra_dispositivo(hardware.es, 4, hardware.console, 4, term_le, NULL);
  es_registra_dispositivo(hardware.es, 5, hardware.console, 5, term_le, NULL);
  es_registra_dispositivo(hardware.es, 6, hardware.console, 6, NULL, term_escr);
  es_registra_dispositivo(hardware.es, 7, hardware.console, 7, term_le, NULL);
  // lê relógio virtual, relógio real
  es_registra_dispositivo(hardware.es, 8, hardware.relogio, 0, rel_le, NULL);
  es_registra_dispositivo(hardware.es, 9, hardware.relogio, 1, rel_le, NULL);

  // cria a unidade de execução e inicializa com a memória e E/S
  hardware.cpu = cpu_cria(hardware.mem, hardware.es);

  // cria o controlador e inicializa com a CPU
  hardware.controle = controle_cria(hardware.cpu, hardware.console, hardware.relogio);
}

void destroi_hardware()
{
  controle_destroi(hardware.controle);
  cpu_destroi(hardware.cpu);
  es_destroi(hardware.es);
  rel_destroi(hardware.relogio);
  console_destroi(hardware.console);
  mem_destroi(hardware.mem);
}

int main()
{
  // cria o hardware
  cria_hardware();
  // coloca um programa na memória
  init_mem(hardware.mem, "ex6.maq");
  
  controle_laco(hardware.controle);
      
  destroi_hardware();
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

#include "controle.h"
#include "programa.h"
#include "memoria.h"
#include "cpu.h"
#include "relogio.h"
#include "console.h"
#include "so.h"

#include <stdio.h>
#include <stdlib.h>

// constantes
#define MEM_TAM 2000        // tamanho da memória principal


typedef struct {
  mem_t *mem;
  cpu_t *cpu;
  relogio_t *relogio;
  console_t *console;
  es_t *es;
  controle_t *controle;
} hardware_t;

void cria_hardware(hardware_t *hw)
{
  // cria a memória
  hw->mem = mem_cria(MEM_TAM);

  // cria dispositivos de E/S
  hw->console = console_cria();
  hw->relogio = rel_cria();

  // cria o controlador de E/S e registra os dispositivos
  hw->es = es_cria();
  // lê teclado, testa teclado, escreve tela, testa tela do terminal A
  es_registra_dispositivo(hw->es, 0, hw->console, 0, term_le, NULL);
  es_registra_dispositivo(hw->es, 1, hw->console, 1, term_le, NULL);
  es_registra_dispositivo(hw->es, 2, hw->console, 2, NULL, term_escr);
  es_registra_dispositivo(hw->es, 3, hw->console, 3, term_le, NULL);
  // lê teclado, testa teclado, escreve tela, testa tela do terminal B
  es_registra_dispositivo(hw->es, 4, hw->console, 4, term_le, NULL);
  es_registra_dispositivo(hw->es, 5, hw->console, 5, term_le, NULL);
  es_registra_dispositivo(hw->es, 6, hw->console, 6, NULL, term_escr);
  es_registra_dispositivo(hw->es, 7, hw->console, 7, term_le, NULL);
  // lê relógio virtual, relógio real
  es_registra_dispositivo(hw->es, 8, hw->relogio, 0, rel_le, NULL);
  es_registra_dispositivo(hw->es, 9, hw->relogio, 1, rel_le, NULL);

  // cria a unidade de execução e inicializa com a memória e E/S
  hw->cpu = cpu_cria(hw->mem, hw->es);

  // cria o controlador e inicializa com a CPU
  hw->controle = controle_cria(hw->cpu, hw->console, hw->relogio);
}

void destroi_hardware(hardware_t *hw)
{
  controle_destroi(hw->controle);
  cpu_destroi(hw->cpu);
  es_destroi(hw->es);
  rel_destroi(hw->relogio);
  console_destroi(hw->console);
  mem_destroi(hw->mem);
}

int main()
{
  hardware_t hw;
  so_t *so;

  // cria o hardware
  cria_hardware(&hw);
  // cria o sistema operacional
  so = so_cria(hw.cpu, hw.mem, hw.console, hw.relogio);
  
  // executa o laço de execução da CPU
  controle_laco(hw.controle);

  // destroi tudo
  so_destroi(so);
  destroi_hardware(&hw);
  return 0;
}


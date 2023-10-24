#include "so.h"
#include "irq.h"
#include "programa.h"
#include "instrucao.h"
#include "processo.h"
#include "processo.c"

#include <stdlib.h>
#include <stdbool.h>
#include<time.h>

// intervalo entre interrupções do relógio
#define INTERVALO_INTERRUPCAO 50   // em instruções executadas

#define MAX_PROCESSOS 10

#define NENHUM_PROCESSO_EM_EXECUCAO -1

struct so_t {
  cpu_t *cpu;
  mem_t *mem;
  console_t *console;
  relogio_t *relogio;
  int pid_processo_em_execucao;
  processo_t *tabela_processos[MAX_PROCESSOS];
};


// função de tratamento de interrupção (entrada no SO)
static err_t so_trata_interrupcao(void *argC, int reg_A);

// funções auxiliares
static int so_carrega_programa(so_t *self, char *nome_do_executavel);
static bool copia_str_da_mem(int tam, char str[tam], mem_t *mem, int ender);

// funções de tratamento de processo
static void inicializa_tabela_processos(so_t *self);
static processo_t* cria_processo(so_t *self);
static int geraPid();
static processo_t* recupera_processo_por_pid(so_t *self, int pid);
static processo_t* recupera_processo_atual(so_t *self);
static processo_t* recupera_primeiro_processo_pronto(so_t *self);
static void adiciona_processo_na_tabela(so_t *self, processo_t *novo_processo);
static int recupera_posicao_livre_tabela_de_processos(so_t *self);
static void mata_processo(so_t *self, int posicao);
static int recupera_posicao_tabela_de_processos(so_t *self, int pid);
// static void exibir_tabela(so_t *self);

so_t *so_cria(cpu_t *cpu, mem_t *mem, console_t *console, relogio_t *relogio)
{
  so_t *self = malloc(sizeof(*self));
  if (self == NULL) return NULL;

  self->cpu = cpu;
  self->mem = mem;
  self->console = console;
  self->relogio = relogio;
  self->pid_processo_em_execucao = NENHUM_PROCESSO_EM_EXECUCAO;
  inicializa_tabela_processos(self);

  // quando a CPU executar uma instrução CHAMAC, deve chamar a função
  //   so_trata_interrupcao
  cpu_define_chamaC(self->cpu, so_trata_interrupcao, self);

  // coloca o tratador de interrupção na memória
  // quando a CPU aceita uma interrupção, passa para modo supervisor, 
  //   salva seu estado à partir do endereço 0, e desvia para o endereço 10
  // colocamos no endereço 10 a instrução CHAMAC, que vai chamar 
  //   so_trata_interrupcao (conforme foi definido acima) e no endereço 11
  //   colocamos a instrução RETI, para que a CPU retorne da interrupção
  //   (recuperando seu estado no endereço 0) depois que o SO retornar de
  //   so_trata_interrupcao.
  mem_escreve(self->mem, 10, CHAMAC);
  mem_escreve(self->mem, 11, RETI);

  // programa o relógio para gerar uma interrupção após INTERVALO_INTERRUPCAO
  rel_escr(self->relogio, 2, INTERVALO_INTERRUPCAO);

  return self;
}

void so_destroi(so_t *self)
{
  cpu_define_chamaC(self->cpu, NULL, NULL);
  free(self);
}


// Tratamento de interrupção

// funções auxiliares para tratar cada tipo de interrupção
static err_t so_trata_irq(so_t *self, int irq);
static err_t so_trata_irq_reset(so_t *self);
static err_t so_trata_irq_err_cpu(so_t *self);
static err_t so_trata_irq_relogio(so_t *self);
static err_t so_trata_irq_desconhecida(so_t *self, int irq);
static err_t so_trata_chamada_sistema(so_t *self);

// funções auxiliares para o tratamento de interrupção
static void so_salva_estado_da_cpu(so_t *self);
static void so_trata_pendencias(so_t *self);
static void so_escalona(so_t *self);
static err_t so_despacha(so_t *self);

// função a ser chamada pela CPU quando executa a instrução CHAMAC
// essa instrução só deve ser executada quando for tratar uma interrupção
// o primeiro argumento é um ponteiro para o SO, o segundo é a identificação
//   da interrupção
// na inicialização do SO é colocada no endereço 10 uma rotina que executa
//   CHAMAC; quando recebe uma interrupção, a CPU salva os registradores
//   no endereço 0, e desvia para o endereço 10
static err_t so_trata_interrupcao(void *argC, int reg_A)
{
  so_t *self = argC;
  irq_t irq = reg_A;
  err_t err = ERR_OK;
  console_printf(self->console, "SO: recebi IRQ %d (%s)", irq, irq_nome(irq));
  // salva o estado da cpu no descritor do processo que foi interrompido
  so_salva_estado_da_cpu(self);
  // faz o atendimento da interrupção
  err = so_trata_irq(self, irq);
  // faz o processamento independente da interrupção
  so_trata_pendencias(self);
  // escolhe o próximo processo a executar
  so_escalona(self);
  // recupera o estado do processo escolhido
  err = so_despacha(self);
  return err;
}

static void so_salva_estado_da_cpu(so_t *self)
{
  // se não houver processo corrente, não faz nada
  if (self->pid_processo_em_execucao == NENHUM_PROCESSO_EM_EXECUCAO) {
    return;
  }
  // salva os registradores que compõem o estado da cpu no descritor do
  //   processo corrente
  processo_t *processo_atual = recupera_processo_atual(self);
  processo_atual->estado_processo = BLOQUEADO;
  // mem_le(self->mem, IRQ_END_A, endereco onde vai o A no descritor);
  // mem_le(self->mem, IRQ_END_X, endereco onde vai o X no descritor);
  // etc
  mem_le(self->mem, IRQ_END_A, &processo_atual->estado_cpu->A);
  mem_le(self->mem, IRQ_END_X, &processo_atual->estado_cpu->X);
  mem_le(self->mem, IRQ_END_complemento, &processo_atual->estado_cpu->complemento);
  mem_le(self->mem, IRQ_END_PC, &processo_atual->estado_cpu->PC);
  mem_le(self->mem, IRQ_END_modo, (int *)&processo_atual->estado_cpu->modo);
  mem_le(self->mem, IRQ_END_erro, &processo_atual->estado_cpu->erro);
}
static void so_trata_pendencias(so_t *self)
{
  // realiza ações que não são diretamente ligadar com a interrupção que
  //   está sendo atendida:
  // - E/S pendente
  // - desbloqueio de processos
  // - contabilidades
}
static void so_escalona(so_t *self)
{
// Define essa variável que pode receber o PID do estado anterior ou do próximo. 
  // Caso não haja processo para escalonar, a variável mantém o valor que define que nenhum processo está em execução.
  int pid_processo_escalonado = NENHUM_PROCESSO_EM_EXECUCAO;

  // Obtém o último processo que estava sendo executado antes da interrupção
  processo_t *processo_anterior = recupera_processo_atual(self);

  // Verifica se ele está PRONTO. Se não, obtém o próximo PRONTO da tabela de processos
  if(processo_anterior != NULL && processo_anterior->estado_processo == PRONTO) {
    self->pid_processo_em_execucao = processo_anterior->pid;
  }else{
    processo_t *proximo_processo = recupera_primeiro_processo_pronto(self);
    if(proximo_processo != NULL){
      pid_processo_escalonado = proximo_processo->pid;
    }
  }

  // escolhe o próximo processo a executar, que passa a ser o processo
  //   corrente; pode continuar sendo o mesmo de antes ou não
  self->pid_processo_em_execucao = pid_processo_escalonado;
  console_printf(self->console, "Processo escalonado: %d", self->pid_processo_em_execucao);
}
static err_t so_despacha(so_t *self)
{
   // se não houver processo corrente, coloca ERR_CPU_PARADA em IRQ_END_erro
  if(self->pid_processo_em_execucao == NENHUM_PROCESSO_EM_EXECUCAO) {
   // Não existe processo para executar
   console_printf(self->console, "Não existe processo para executar ERR_CPU_PARADA");
   return ERR_CPU_PARADA; 
  }

  // se houver processo corrente, coloca todo o estado desse processo em
  //   IRQ_END_*
  processo_t *processo_escalonado = recupera_processo_atual(self);
  mem_escreve(self->mem, IRQ_END_A, (processo_escalonado->estado_cpu->A));
  mem_escreve(self->mem, IRQ_END_X, (processo_escalonado->estado_cpu->X));
  mem_escreve(self->mem, IRQ_END_complemento, (processo_escalonado->estado_cpu->complemento));
  mem_escreve(self->mem, IRQ_END_PC, (processo_escalonado->estado_cpu->PC));
  mem_escreve(self->mem, IRQ_END_modo, (processo_escalonado->estado_cpu->modo));
  mem_escreve(self->mem, IRQ_END_erro, (processo_escalonado->estado_cpu->erro));
  return ERR_OK;
}

static err_t so_trata_irq(so_t *self, int irq)
{
  err_t err;
  console_printf(self->console, "SO: recebi IRQ %d (%s)", irq, irq_nome(irq));
  switch (irq) {
    case IRQ_RESET:
      err = so_trata_irq_reset(self);
      break;
    case IRQ_ERR_CPU:
      err = so_trata_irq_err_cpu(self);
      break;
    case IRQ_SISTEMA:
      err = so_trata_chamada_sistema(self);
      break;
    case IRQ_RELOGIO:
      err = so_trata_irq_relogio(self);
      break;
    default:
      err = so_trata_irq_desconhecida(self, irq);
  }
  return err;
}

static err_t so_trata_irq_reset(so_t *self)
{

  console_printf(self->console, "so_trata_irq_reset");
  inicializa_tabela_processos(self);
  // coloca um programa na memória
  int ender = so_carrega_programa(self, "init.maq");
  if (ender != 100) {
    console_printf(self->console, "SO: problema na carga do programa inicial");
    return ERR_CPU_PARADA;
  }

  // deveria criar um processo para o init, e inicializar o estado do
  //   processador para esse processo com os registradores zerados, exceto
  //   o PC e o modo.
  // como não tem suporte a processos, está carregando os valores dos
  //   registradores diretamente para a memória, de onde a CPU vai carregar
  //   para os seus registradores quando executar a instrução RETI

  processo_t *processo_init = cria_processo(self);
  // escreve o endereço de carga no pc do descritor do processo
  // mem_escreve(self->mem, processo_init->estado_cpu->PC, ender); //verificar isso
  adiciona_processo_na_tabela(self, processo_init);

  // altera o PC para o endereço de carga (deve ter sido 100)
  mem_escreve(self->mem, IRQ_END_PC, ender);
  // passa o processador para modo usuário
  mem_escreve(self->mem, IRQ_END_modo, usuario);
  return ERR_OK;
}

static err_t so_trata_irq_err_cpu(so_t *self)
{
  // Ocorreu um erro interno na CPU
  // O erro está codificado em IRQ_END_erro
  // Em geral, causa a morte do processo que causou o erro
  // Ainda não temos processos, causa a parada da CPU
  int err_int;
  // com suporte a processos, deveria pegar o valor do registrador erro
  //   no descritor do processo corrente, e reagir de acordo com esse erro
  //   (em geral, matando o processo)
  // mem_le(self->mem, IRQ_END_erro, &err_int);

  processo_t *processo_atual = recupera_processo_atual(self);

  mem_le(self->mem, processo_atual->estado_cpu->erro, &err_int);
  console_printf(self->console, "Lendo erro %d",err_int);
  err_t err = err_int;
  console_printf(self->console,
        "SO: Tratando -- erro na CPU: %s", err_nome(err));

  mata_processo(self, processo_atual->pid); // Estou tratando todos os erros com a morte do processo. Devo tratar de outra forma?

  return ERR_OK;
}

static err_t so_trata_irq_relogio(so_t *self)
{
  // ocorreu uma interrupção do relógio
  // rearma o interruptor do relógio e reinicializa o timer para a próxima interrupção
  rel_escr(self->relogio, 3, 0); // desliga o sinalizador de interrupção
  rel_escr(self->relogio, 2, INTERVALO_INTERRUPCAO);
  // trata a interrupção
  // por exemplo, decrementa o quantum do processo corrente, quando se tem
  // um escalonador com quantum
  console_printf(self->console, "SO: interrupção do relógio (não tratada)");
  return ERR_OK;
}

static err_t so_trata_irq_desconhecida(so_t *self, int irq)
{
  console_printf(self->console,
      "SO: não sei tratar IRQ %d (%s)", irq, irq_nome(irq));
  return ERR_CPU_PARADA;
}

// Chamadas de sistema

static void so_chamada_le(so_t *self);
static void so_chamada_escr(so_t *self);
static void so_chamada_cria_proc(so_t *self);
static void so_chamada_mata_proc(so_t *self);

static err_t so_trata_chamada_sistema(so_t *self)
{
  // com processos, a identificação da chamada está no reg A no descritor
  //   do processo
  int id_chamada;
  mem_le(self->mem, IRQ_END_A, &id_chamada);
  console_printf(self->console,
      "SO: chamada de sistema %d", id_chamada);
  switch (id_chamada) {
    case SO_LE:
      so_chamada_le(self);
      break;
    case SO_ESCR:
      so_chamada_escr(self);
      break;
    case SO_CRIA_PROC:
      so_chamada_cria_proc(self);
      break;
    case SO_MATA_PROC:
      so_chamada_mata_proc(self);
      break;
    default:
      console_printf(self->console,
          "SO: chamada de sistema desconhecida (%d)", id_chamada);
      return ERR_CPU_PARADA;
  }
  return ERR_OK;
}

static void so_chamada_le(so_t *self)
{
  // implementação com espera ocupada
  //   deveria bloquear o processo se leitura não disponível.
  //   no caso de bloqueio do processo, a leitura (e desbloqueio) deverá
  //   ser feita mais tarde, em tratamentos pendentes em outra interrupção,
  //   ou diretamente em uma interrupção específica do dispositivo, se for
  //   o caso
  // implementação lendo direto do terminal A
  //   deveria usar dispositivo corrente de entrada do processo
  for (;;) {
    int estado;
    term_le(self->console, 1, &estado);
    if (estado != 0) break;
    // como não está saindo do SO, o laço do processador não tá rodando
    // esta gambiarra faz o console andar
    // com a implementação de bloqueio de processo, esta gambiarra não
    //   deve mais existir.
    console_tictac(self->console);
    console_atualiza(self->console);
  }
  int dado;
  term_le(self->console, 0, &dado);
  // com processo, deveria escrever no reg A do processo
  mem_escreve(self->mem, IRQ_END_A, dado);
}

static void so_chamada_escr(so_t *self)
{
  // implementação com espera ocupada
  //   deveria bloquear o processo se dispositivo ocupado
  // implementação escrevendo direto do terminal A
  //   deveria usar dispositivo corrente de saída do processo
  for (;;) {
    int estado;
    term_le(self->console, 3, &estado);
    if (estado != 0) break;
    // como não está saindo do SO, o laço do processador não tá rodando
    // esta gambiarra faz o console andar
    console_tictac(self->console);
    console_atualiza(self->console);
  }
  int dado;
  mem_le(self->mem, IRQ_END_X, &dado);
  term_escr(self->console, 2, dado);
  mem_escreve(self->mem, IRQ_END_A, 0);
}

static void so_chamada_cria_proc(so_t *self)
{
  console_printf(self->console, "so_chamada_cria_proc");
  // ainda sem suporte a processos, carrega programa e passa a executar ele
  // quem chamou o sistema não vai mais ser executado, coitado!

  //Recupera o processo atual. É ele que gerou uma chamada de sistema para criação de outro processo.
  processo_t *processo_pai = recupera_processo_por_pid(self, self->pid_processo_em_execucao);


  // em X está o endereço onde está o nome do arquivo
  int ender_proc;
  // deveria ler o X do descritor do processo criador
  if (mem_le(self->mem, processo_pai->estado_cpu->X, &ender_proc) == ERR_OK) {
    char nome[100];
    if (copia_str_da_mem(100, nome, self->mem, ender_proc)) {
      int ender_carga = so_carrega_programa(self, nome);
      if (ender_carga > 0) {
        // cria novo processo
        processo_t *novo_processo = cria_processo(self);
        // escreve o endereço de carga no pc do descritor do processo
         mem_escreve(self->mem, novo_processo->estado_cpu->PC, ender_carga);
        adiciona_processo_na_tabela(self, novo_processo);
        return;
      }
    }
  }
  // deveria escrever -1 (se erro) ou o PID do processo criado (se OK) no reg A
  //   do processo que pediu a criação
  mem_escreve(self->mem, IRQ_END_A, -1);
}

static void so_chamada_mata_proc(so_t *self)
{
  console_printf(self->console, "so_chamada_mata_proc");
  /*
  O processo pode ser morto por 4 motivos:
    - Saída normal (voluntária): programa terminou ou se matou
    - Erro fatal (involuntária): comando inválido
    - Saída por erro (voluntária): erro de programa, instrução ilegal, memoria inexistente
    - Morto por outro processo (involuntário)
  */

  //Lê do registrador A
  int pid_processo_a_ser_morto = -1;
  mem_le(self->mem, IRQ_END_A, &pid_processo_a_ser_morto);
  mata_processo(self, pid_processo_a_ser_morto);

  // ainda sem suporte a processos, retorna erro -1
  // console_printf(self->console, "SO: SO_MATA_PROC não implementada");
  // mem_escreve(self->mem, IRQ_END_A, -1);
}


// carrega o programa na memória
// retorna o endereço de carga ou -1
static int so_carrega_programa(so_t *self, char *nome_do_executavel)
{
  console_printf(self->console, "so_carrega_programa");
  // programa para executar na nossa CPU
  programa_t *prog = prog_cria(nome_do_executavel);
  if (prog == NULL) {
    console_printf(self->console,
        "Erro na leitura do programa '%s'\n", nome_do_executavel);
    return -1;
  }

  int end_ini = prog_end_carga(prog);
  int end_fim = end_ini + prog_tamanho(prog);

  for (int end = end_ini; end < end_fim; end++) {
    if (mem_escreve(self->mem, end, prog_dado(prog, end)) != ERR_OK) {
      console_printf(self->console,
          "Erro na carga da memória, endereco %d\n", end);
      return -1;
    }
  }
  prog_destroi(prog);
  console_printf(self->console,
      "SO: carga de '%s' em %d-%d", nome_do_executavel, end_ini, end_fim);
  return end_ini;
}

// copia uma string da memória do simulador para o vetor str.
// retorna false se erro (string maior que vetor, valor não ascii na memória,
//   erro de acesso à memória)
static bool copia_str_da_mem(int tam, char str[tam], mem_t *mem, int ender)
{
  for (int indice_str = 0; indice_str < tam; indice_str++) {
    int caractere;
    if (mem_le(mem, ender + indice_str, &caractere) != ERR_OK) {
      return false;
    }
    if (caractere < 0 || caractere > 255) {
      return false;
    }
    str[indice_str] = caractere;
    if (caractere == 0) {
      return true;
    }
  }
  // estourou o tamanho de str
  return false;
}

// Funções de tratamento de processo

static void inicializa_tabela_processos(so_t *self){
  for (int i = 0; i < MAX_PROCESSOS; i++) {
        self->tabela_processos[i] = malloc(sizeof(processo_t));
        self->tabela_processos[i]->livre = true;
    }
}

static processo_t* cria_processo(so_t *self) {
  processo_t *novo_processo = malloc(sizeof(processo_t));
  if(novo_processo != NULL) {
    novo_processo->estado_cpu = malloc(sizeof(registros_t));
    novo_processo->pid = geraPid();
    novo_processo->estado_processo = PRONTO;
    novo_processo->estado_cpu->modo = usuario;
    novo_processo->estado_cpu->complemento = 0;
    novo_processo->estado_cpu->X = 0;
    novo_processo->estado_cpu->erro = ERR_OK;
    novo_processo->livre = false;
  }
  return novo_processo;
}

static int geraPid() {
  srand(time(NULL));
  return rand() % MAX_PROCESSOS;
}

// Recupera um processo com base em algum PID
static processo_t* recupera_processo_por_pid(so_t *self, int pid) {
  for (int i=0; i<MAX_PROCESSOS; i++) {
    if(self->tabela_processos[i]->pid == pid) {
      return self->tabela_processos[i];
    }
  }

  return NULL;
}

// Recupera o processo atual com base no pid_processo_em_execucao
static processo_t* recupera_processo_atual(so_t *self) {
    return recupera_processo_por_pid(self, self->pid_processo_em_execucao);
}

// Recupera o primeiro processo que estiver com o estado PRONTO
static processo_t* recupera_primeiro_processo_pronto(so_t *self) {
  for(int i=0; i<MAX_PROCESSOS; i++) {
    if(self->tabela_processos[i]->estado_processo == PRONTO){
      return self->tabela_processos[i];
    }
  }

  return NULL;
}

static void adiciona_processo_na_tabela(so_t *self, processo_t *novo_processo) {
  int posicao_livre_tabela_processos = recupera_posicao_livre_tabela_de_processos(self);
  if(posicao_livre_tabela_processos != -1) {
    self->tabela_processos[posicao_livre_tabela_processos] = novo_processo;
  }else{
    console_printf(self->console, "Tabela cheia");
    // Tabela de processos cheia. O que fazer?
  }
}

// Recupera o primeiro processo que estiver com o estado PRONTO
static int recupera_posicao_livre_tabela_de_processos(so_t *self) {
  for(int i=0; i<MAX_PROCESSOS; i++) {
    if(self->tabela_processos[i]->livre){
      return i;
    }
  }
  return -1;
}

// Recupera posição do processo com base no PID
static int recupera_posicao_tabela_de_processos(so_t *self, int pid) {
  for(int i=0; i<MAX_PROCESSOS; i++) {
    if(self->tabela_processos[i]->pid == pid){
      return i;
    }
  }
  return -1;
}

static void mata_processo(so_t *self, int pid) {
  int i = recupera_posicao_tabela_de_processos(self, pid);
  if(i != - 1){
    self->tabela_processos[i] = malloc(sizeof(processo_t));
    self->tabela_processos[i]->livre = true;
  }else{
    console_printf(self->console, "Processo não encontrado");
  }
}

// static void exibir_tabela(so_t *self) {
//   for(int i=0; i<MAX_PROCESSOS; i++) {
//     if(!self->tabela_processos[i]->livre)
//     console_printf(self->console, "pid: %d", self->tabela_processos[i]->pid);
//   }
// }
#include "so.h"
#include "irq.h"
#include "programa.h"
#include "instrucao.h"

#include <stdlib.h>
#include <stdbool.h>

// intervalo entre interrupções do relógio
#define INTERVALO_INTERRUPCAO 50 // em instruções executadas

#define MAX_PROCESSOS 10

#define NENHUM_PROCESSO_EM_EXECUCAO -1

typedef enum
{
  EXECUCAO,
  PRONTO,
  BLOQUEADO
} estado_processo;

struct registros_t
{
  int PC;
  int A;
  int X;
  int complemento;
  int erro;
  cpu_modo_t modo;
};

struct processo_t
{
  estado_processo estado_processo;
  int pid;
  int pid_processo_pai;
  registros_t estado_cpu;
  bool livre;
};

/*
  Identifica o tipo de pendência na estrutura pendencia_t
*/
typedef enum
{
  ENTRADA,
  SAIDA,
  ESPERA_PROC
} tipo_pendencia;

struct pendencia_t
{
  /*
    PID do processo que causou a pendência
  */
  int pid;
  /*
    Caso o tipo_pendencia seja ENTRADA ou SAÍDA, pid_espera_proc terá o valor -1
    Caso o tipo_pendencia seja ESPERA_PROC, dispositivo terá o valor -1
  */
  int dispositivo;
  int pid_espera_proc;
  tipo_pendencia tipo_pendencia;
};

struct so_t
{
  cpu_t *cpu;
  mem_t *mem;
  console_t *console;
  relogio_t *relogio;
  int pid_processo_em_execucao;
  int count_processos;
  processo_t tabela_processos[MAX_PROCESSOS];
  /*
    pendencias_es
    Armazena as solicitações de E/S quando um dispositivo estiver ocupado com uma struct.
    int dispositivo - Id do dispositivo de E/S (DL, EL, DE, EE)
    int pid - PID do processo que solicitou a E/S
  */
  pendencia_t pendencias_es[MAX_PROCESSOS];
};

// função de tratamento de interrupção (entrada no SO)
static err_t so_trata_interrupcao(void *argC, int reg_A);
static void coloca_cpu_em_estado_parada(so_t *self);

// funções auxiliares
static int so_carrega_programa(so_t *self, char *nome_do_executavel);
static bool copia_str_da_mem(int tam, char str[tam], mem_t *mem, int ender);

// funções de tratamento de processo
static void inicializa_tabela_processos(so_t *self);
static int cria_processo(so_t *self, int ender_carga, int pid_processo_pai);
static int geraPid(so_t *self);
static int recupera_processo_por_pid(so_t *self, int pid);
static int recupera_processo_atual(so_t *self);
static int recupera_posicao_primeiro_processo_pronto(so_t *self);
static int adiciona_processo_na_tabela(so_t *self, processo_t novo_processo);
static int recupera_posicao_livre_tabela_de_processos(so_t *self);
static void mata_processo(so_t *self, int posicao);
static int recupera_posicao_tabela_de_processos(so_t *self, int pid);
static void desbloqueia_processo(so_t *self, int pid);
static bool existe_processo(so_t *self, int pid);

// função de tratamento de terminal/dispositivo
static int obter_terminal_por_pid(int pid);
static int obter_id_por_term(int term, int op);

// funções para tratamento de pendências
static void inicializa_pendencias_es(so_t *self);
static int obter_posicao_livre_pendencias_es(so_t *self);
static void so_trata_pendencia_es(so_t *self);
static void adiciona_pendencia(so_t *self, int id, int pid, tipo_pendencia tipo_pendencia);
static void remove_pendencia_es(so_t *self, int i);
static void trata_pendencia_leitura(so_t *self, int i);
static void trata_pendencia_escrita(so_t *self, int i);
static void trata_pendencia_espera_proc(so_t *self, int i);

// funções para entrada e saída
static bool verifica_estado_es(so_t *self, int id);
static void atende_leitura(so_t *self, int term, int pid);
static void atende_escrita(so_t *self, int term);

so_t *so_cria(cpu_t *cpu, mem_t *mem, console_t *console, relogio_t *relogio)
{
  so_t *self = malloc(sizeof(*self));
  if (self == NULL)
    return NULL;

  self->cpu = cpu;
  self->mem = mem;
  self->console = console;
  self->relogio = relogio;
  self->count_processos = 0;
  self->pid_processo_em_execucao = NENHUM_PROCESSO_EM_EXECUCAO;
  inicializa_tabela_processos(self);
  inicializa_pendencias_es(self);

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
static void so_despacha(so_t *self);

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
  err_t err;
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
  so_despacha(self);
  return err;
}

static void so_salva_estado_da_cpu(so_t *self)
{
  // se não houver processo corrente, não faz nada
  if (self->pid_processo_em_execucao == NENHUM_PROCESSO_EM_EXECUCAO)
  {
    return;
  }
  // salva os registradores que compõem o estado da cpu no descritor do
  //   processo corrente
  int processo_atual = recupera_processo_atual(self);
  mem_le(self->mem, IRQ_END_A, &self->tabela_processos[processo_atual].estado_cpu.A);
  mem_le(self->mem, IRQ_END_X, &self->tabela_processos[processo_atual].estado_cpu.X);
  mem_le(self->mem, IRQ_END_complemento, &self->tabela_processos[processo_atual].estado_cpu.complemento);
  mem_le(self->mem, IRQ_END_PC, &self->tabela_processos[processo_atual].estado_cpu.PC);
  mem_le(self->mem, IRQ_END_modo, (int *)&self->tabela_processos[processo_atual].estado_cpu.modo);
  mem_le(self->mem, IRQ_END_erro, &self->tabela_processos[processo_atual].estado_cpu.erro);
}
static void so_trata_pendencias(so_t *self)
{
  // realiza ações que não são diretamente ligadas com a interrupção que
  //   está sendo atendida:
  // - E/S pendente
  // - desbloqueio de processos
  // - contabilidades
  so_trata_pendencia_es(self);
}

static void so_trata_pendencia_es(so_t *self)
{
  for (int i = 0; i < MAX_PROCESSOS; i++) {
    pendencia_t pendencia = self->pendencias_es[i];

    if (pendencia.pid != -1) {

      if (pendencia.tipo_pendencia == ENTRADA) {
        trata_pendencia_leitura(self, i);
      }

      else if (pendencia.tipo_pendencia == SAIDA) {
        trata_pendencia_escrita(self, i);
      }

      else if (pendencia.tipo_pendencia == ESPERA_PROC) {
        trata_pendencia_espera_proc(self, i);
      }
    }
  }
}

static void trata_pendencia_leitura(so_t *self, int i)
{
  int dispositivo = self->pendencias_es[i].dispositivo;
  int pid = self->pendencias_es[i].pid;
  bool disponivel = verifica_estado_es(self, dispositivo);

  if (disponivel) {
    int term = obter_terminal_por_pid(pid);
    atende_leitura(self, term, pid);
    desbloqueia_processo(self, pid);
    remove_pendencia_es(self, i);
  }
}

static void trata_pendencia_escrita(so_t *self, int i) {
  int dispositivo = self->pendencias_es[i].dispositivo;
  int pid = self->pendencias_es[i].pid;
  bool disponivel = verifica_estado_es(self, dispositivo);

  if (disponivel) {
    int term = obter_terminal_por_pid(pid);
    atende_escrita(self, term);
    desbloqueia_processo(self, pid);
    remove_pendencia_es(self, i);
  }
}

static void trata_pendencia_espera_proc(so_t *self, int i) {
  int pid = self->pendencias_es[i].pid;
  int pid_espera_proc = self->pendencias_es[i].pid_espera_proc;

  bool processo_vivo = existe_processo(self, pid_espera_proc);

  if (!processo_vivo) {
    desbloqueia_processo(self, pid);
    remove_pendencia_es(self, i);
  }
}

static void remove_pendencia_es(so_t *self, int i)
{
  self->pendencias_es[i].dispositivo = -1;
  self->pendencias_es[i].pid = -1;
  self->pendencias_es[i].pid_espera_proc = -1;
}

/*
  Verifica se um dispositivo de E/S está disponível.
  Retorna 'tru'e se sim, 'false' se estiver ocupado.
*/
static bool verifica_estado_es(so_t *self, int id)
{
  int estado;
  term_le(self->console, id, &estado);
  return estado != 0;
}

/*
  Desbloqueia um processo, colocando o estado dele como PRONTO.
*/
static void desbloqueia_processo(so_t *self, int pid)
{
  int i = recupera_processo_por_pid(self, pid);
  self->tabela_processos[i].estado_processo = PRONTO;
}

static void so_escalona(so_t *self)
{
  // Define essa variável que pode receber o PID do estado anterior ou do próximo.
  // Caso não haja processo para escalonar, a variável mantém o valor que define que nenhum processo está em execução.
  int pid_processo_escalonado = NENHUM_PROCESSO_EM_EXECUCAO;

  // Obtém o último processo que estava sendo executado antes da interrupção
  int i_processo_anterior = recupera_processo_atual(self);
  processo_t processo_anterior = self->tabela_processos[i_processo_anterior];

  // Verifica se ele está PRONTO OU EXECUTANDO. Se não, obtém o próximo PRONTO da tabela de processos
  if (i_processo_anterior != -1 && (processo_anterior.estado_processo == PRONTO || processo_anterior.estado_processo == EXECUCAO)) {
    pid_processo_escalonado = processo_anterior.pid;
  } else {
    int proximo_processo = recupera_posicao_primeiro_processo_pronto(self);
    if (proximo_processo != -1) {
      pid_processo_escalonado = self->tabela_processos[proximo_processo].pid;
    } else {
      // Existem processos mas nenhum está pronto
      coloca_cpu_em_estado_parada(self);
    }
  }

  // escolhe o próximo processo a executar, que passa a ser o processo
  //   corrente; pode continuar sendo o mesmo de antes ou não
  self->pid_processo_em_execucao = pid_processo_escalonado;
  
}
static void so_despacha(so_t *self)
{
  // se não houver processo corrente, coloca ERR_CPU_PARADA em IRQ_END_erro
  if (self->pid_processo_em_execucao == NENHUM_PROCESSO_EM_EXECUCAO)
  {
    // Não existe processo para executar
    coloca_cpu_em_estado_parada(self);
  }
  else
  {
    // se houver processo corrente, coloca todo o estado desse processo em
    //   IRQ_END_*
    int processo_escalonado = recupera_processo_atual(self);
    mem_escreve(self->mem, IRQ_END_A, (self->tabela_processos[processo_escalonado].estado_cpu.A));
    mem_escreve(self->mem, IRQ_END_X, (self->tabela_processos[processo_escalonado].estado_cpu.X));
    mem_escreve(self->mem, IRQ_END_complemento, (self->tabela_processos[processo_escalonado].estado_cpu.complemento));
    mem_escreve(self->mem, IRQ_END_PC, (self->tabela_processos[processo_escalonado].estado_cpu.PC));
    mem_escreve(self->mem, IRQ_END_modo, (self->tabela_processos[processo_escalonado].estado_cpu.modo));
    mem_escreve(self->mem, IRQ_END_erro, (self->tabela_processos[processo_escalonado].estado_cpu.erro));
    // Define estado do processo para EXECUCAO
    self->tabela_processos[processo_escalonado].estado_processo = EXECUCAO;
  }
}

static void coloca_cpu_em_estado_parada(so_t *self)
{
  mem_escreve(self->mem, IRQ_END_erro, ERR_CPU_PARADA);
  mem_escreve(self->mem, IRQ_END_modo, usuario);
  console_printf(self->console, "Não existe nenhum processo PRONTO para executar ERR_CPU_PARADA");
}

static err_t so_trata_irq(so_t *self, int irq)
{
  err_t err;
  console_printf(self->console, "SO: recebi IRQ %d (%s)", irq, irq_nome(irq));
  switch (irq)
  {
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
  inicializa_tabela_processos(self);
  // coloca um programa na memória
  int ender = so_carrega_programa(self, "init.maq");
  if (ender != 100)
  {
    console_printf(self->console, "SO: problema na carga do programa inicial");
    return ERR_CPU_PARADA;
  }

  // deveria criar um processo para o init, e inicializar o estado do
  //   processador para esse processo com os registradores zerados, exceto
  //   o PC e o modo.
  // como não tem suporte a processos, está carregando os valores dos
  //   registradores diretamente para a memória, de onde a CPU vai carregar
  //   para os seus registradores quando executar a instrução RETI

  // cria processo; escreve o endereço de carga no pc do descritor do processo; e muda o modo para usuário
  cria_processo(self, ender, -1);

  return ERR_OK;
}

static err_t so_trata_irq_err_cpu(so_t *self)
{
  // Ocorreu um erro interno na CPU
  // O erro está codificado em IRQ_END_erro
  // Em geral, causa a morte do processo que causou o erro
  // Ainda não temos processos, causa a parada da CPU

  // com suporte a processos, deveria pegar o valor do registrador erro
  //   no descritor do processo corrente, e reagir de acordo com esse erro
  //   (em geral, matando o processo)
  // mem_le(self->mem, IRQ_END_erro, &err_int);

  int processo_atual = recupera_processo_atual(self);

  int err_int = self->tabela_processos[processo_atual].estado_cpu.erro;
  console_printf(self->console, "Lendo erro %d", err_int);
  err_t err = err_int;
  console_printf(self->console,
                 "SO: Tratando -- erro na CPU: %s", err_nome(err));

  mata_processo(self, self->pid_processo_em_execucao); // Estou tratando todos os erros com a morte do processo. Devo tratar de outra forma?

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
static void so_chamada_espera_proc(so_t *self);

static err_t so_trata_chamada_sistema(so_t *self)
{
  // com processos, a identificação da chamada está no reg A no descritor
  //   do processo
  int posicao = recupera_processo_atual(self);
  int id_chamada = self->tabela_processos[posicao].estado_cpu.A; // mem_le(self->mem, IRQ_END_A, &id_chamada);

  console_printf(self->console,
                 "SO: chamada de sistema %d", id_chamada);
  switch (id_chamada)
  {
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
  case SO_ESPERA_PROC:
    so_chamada_espera_proc(self);
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
  int i_processo = recupera_processo_atual(self);
  int term = obter_terminal_por_pid(self->tabela_processos[i_processo].pid);

  int estado;
  int id_el = obter_id_por_term(term, 1);
  term_le(self->console, id_el, &estado);
  // Dispositivo disponível - Atende solicitação
  if (estado != 0){
    atende_leitura(self, term, self->tabela_processos[i_processo].pid);
  }
  else { // Dispositivo indisponível - Bloqueia processo
    self->tabela_processos[i_processo].estado_processo = BLOQUEADO;
    adiciona_pendencia(self, id_el, self->tabela_processos[i_processo].pid, ENTRADA);
  }
}

static void so_chamada_escr(so_t *self)
{
  int i_processo = recupera_processo_atual(self);
  int term = obter_terminal_por_pid(self->tabela_processos[i_processo].pid);

  int estado;
  int id_ee = obter_id_por_term(term, 3);
  term_le(self->console, id_ee, &estado);
  // Dispositivo disponível - Atende solicitação
  if (estado != 0) {
    console_printf(self->console, "ENTRADA ATENDIDA!");
    atende_escrita(self, term);
  } else { // Dispositivo indisponível - Bloqueia processo
      console_printf(self->console, "DISPOSITIVO INDISPONIVEL - PROCESSO BLOQUEADO!");
    self->tabela_processos[i_processo].estado_processo = BLOQUEADO;
    adiciona_pendencia(self, id_ee, self->tabela_processos[i_processo].pid, SAIDA);
  }
}

/*
  Faz a leitura de um dispositivo de E/S.
  Essa função só pode ser chamada antes de verificar o estado do dispositivo. Ele deve estar pronto para leitura.
*/
static void atende_leitura(so_t *self, int term, int pid)
{
  int i_processo = recupera_processo_por_pid(self, pid);
  int dado;
  int id_dl = obter_id_por_term(term, 0);
  term_le(self->console, id_dl, &dado);
  self->tabela_processos[i_processo].estado_cpu.A = dado;
}

/*
  Faz a escrita em um dispositivo de E/S.
  Essa função só pode ser chamada antes de verificar o estado do dispositivo. Ele deve estar pronto para escrita.
*/
static void atende_escrita(so_t *self, int term)
{
  int dado;
  mem_le(self->mem, IRQ_END_X, &dado);
  int id_de = obter_id_por_term(term, 2);
  term_escr(self->console, id_de, dado);
  mem_escreve(self->mem, IRQ_END_A, 0);
}

static void so_chamada_cria_proc(so_t *self)
{
  // Recupera o processo atual. É ele que gerou uma chamada de sistema para criação de outro processo.
  int processo_pai = recupera_processo_por_pid(self, self->pid_processo_em_execucao);

  // em X está o endereço onde está o nome do arquivo
  int ender_proc = self->tabela_processos[processo_pai].estado_cpu.X;
  // deveria ler o X do descritor do processo criador
  char nome[100];
  if (copia_str_da_mem(100, nome, self->mem, ender_proc)) {
    int ender_carga = so_carrega_programa(self, nome);
    if (ender_carga > 0) {
      // cria novo processo
      int pid = cria_processo(self, ender_carga, self->pid_processo_em_execucao);
      self->tabela_processos[processo_pai].estado_cpu.A = pid;
    } else {
      self->tabela_processos[processo_pai].estado_cpu.A = -1;
      return;
    }
  }

  // deveria escrever -1 (se erro) ou 0 (se OK) no reg A do processo que
  //   pediu a criação
  self->tabela_processos[processo_pai].estado_cpu.A = 0;
}

static void so_chamada_mata_proc(so_t *self)
{
  /*
  O processo pode ser morto por 4 motivos:
    - Saída normal (voluntária): programa terminou ou se matou
    - Erro fatal (involuntária): comando inválido
    - Saída por erro (voluntária): erro de programa, instrução ilegal, memoria inexistente
    - Morto por outro processo (involuntário)
  */

  // Lê o X do processo chamador
  int posicao_processo_chamador = recupera_processo_atual(self);
  int pid_processo_a_ser_morto = self->tabela_processos[posicao_processo_chamador].estado_cpu.X;
  if (pid_processo_a_ser_morto == 0) {
    mata_processo(self, self->tabela_processos[posicao_processo_chamador].pid);
  }
  else {
    mata_processo(self, pid_processo_a_ser_morto);
  }

  // Retorna sucessso
  mem_escreve(self->mem, IRQ_END_A, 0);
}

static void so_chamada_espera_proc(so_t *self)
{
  // bloquear processo
  int processo_solicitante = recupera_processo_atual(self);
  int pid_espera_proc = self->tabela_processos[processo_solicitante].estado_cpu.X;
  bool existe = existe_processo(self, pid_espera_proc);

  if (existe){
    self->tabela_processos[processo_solicitante].estado_cpu.A = 0;
  }
  else {
    self->tabela_processos[processo_solicitante].estado_cpu.A = -1;
    console_printf(self->console, "Processo %d não existe!", pid_espera_proc);
  }
}

// carrega o programa na memória
// retorna o endereço de carga ou -1
static int so_carrega_programa(so_t *self, char *nome_do_executavel)
{
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

static void inicializa_tabela_processos(so_t *self)
{
  for (int i = 0; i < MAX_PROCESSOS; i++) {
    self->tabela_processos[i].livre = true;
  }
}

static int cria_processo(so_t *self, int ender_carga, int pid_processo_pai)
{
  processo_t novo_processo;
  novo_processo.pid = geraPid(self);
  novo_processo.pid_processo_pai = pid_processo_pai;
  novo_processo.estado_processo = PRONTO;
  // escreve o endereço de carga no pc do descritor do processo
  novo_processo.estado_cpu.PC = ender_carga;
  novo_processo.estado_cpu.modo = usuario;
  novo_processo.estado_cpu.complemento = 0;
  novo_processo.estado_cpu.X = 0;
  novo_processo.estado_cpu.erro = ERR_OK;
  novo_processo.livre = false;
  int posicao = adiciona_processo_na_tabela(self, novo_processo);
  return novo_processo.pid;
}

static int geraPid(so_t *self)
{
  self->count_processos++;
  return self->count_processos;
}

// Recupera um processo com base em algum PID
static int recupera_processo_por_pid(so_t *self, int pid)
{
  for (int i = 0; i < MAX_PROCESSOS; i++) {
    if (self->tabela_processos[i].pid == pid && !self->tabela_processos[i].livre) {
      return i;
    }
  }

  return -1;
}

// Recupera o processo atual com base no pid_processo_em_execucao
static int recupera_processo_atual(so_t *self)
{
  return recupera_processo_por_pid(self, self->pid_processo_em_execucao);
}

// Recupera o primeiro processo que estiver com o estado PRONTO
static int recupera_posicao_primeiro_processo_pronto(so_t *self)
{
  for (int i = 0; i < MAX_PROCESSOS; i++) {
    if (self->tabela_processos[i].estado_processo == PRONTO && !self->tabela_processos[i].livre) {
      return i;
    }
  }

  return -1;
}

static int adiciona_processo_na_tabela(so_t *self, processo_t novo_processo)
{
  int posicao = recupera_posicao_livre_tabela_de_processos(self);
  if (posicao != -1)
  {
    self->tabela_processos[posicao] = novo_processo;
  }
  else
  {
    console_printf(self->console, "Tabela cheia");
    // Tabela de processos cheia. O que fazer?
  }

  return posicao;
}

// Recupera o primeiro processo que estiver com o estado PRONTO
static int recupera_posicao_livre_tabela_de_processos(so_t *self)
{
  for (int i = 0; i < MAX_PROCESSOS; i++)
  {
    if (self->tabela_processos[i].livre)
    {
      return i;
    }
  }
  return -1;
}

// Recupera posição do processo com base no PID
static int recupera_posicao_tabela_de_processos(so_t *self, int pid)
{
  for (int i = 0; i < MAX_PROCESSOS; i++)
  {
    if (self->tabela_processos[i].pid == pid && !self->tabela_processos[i].livre)
    {
      return i;
    }
  }
  return -1;
}

static void mata_processo(so_t *self, int pid)
{
  int i = recupera_posicao_tabela_de_processos(self, pid);
  if (i != -1)
  {
    self->tabela_processos[i].livre = true;
  }
  else
  {
    console_printf(self->console, "Processo não encontrado");
  }
}

static bool existe_processo(so_t *self, int pid)
{
  for (int i = 0; i < MAX_PROCESSOS; i++)
  {
    if (self->tabela_processos[i].pid == pid && !self->tabela_processos[i].livre)
    {
      return true;
    }
  }
  return false;
}

static int obter_terminal_por_pid(int pid)
{
  return ((pid - 1) % 4);
}

static int obter_id_por_term(int term, int op)
{
  return term * 4 + op;
}

// /*
//   Essa função retorna a operação pelo id do dispositivo.
//   Por exemplo, se eu possui um id de dispositivo 15, a função retornará 3 (que identifica uma operação de escrita).
//   Retorna 1, quando é leitura, 3 quando é escrita.
// */
// static int obter_op_por_id(int id, int pid)
// {
//   int term = obter_terminal_por_pid(pid);
//   return id - (term * 4);
// }

// Tratamento de pendências
static void inicializa_pendencias_es(so_t *self)
{
  for (int i = 0; i < MAX_PROCESSOS; i++)
  {
    self->pendencias_es[i].dispositivo = -1;
    self->pendencias_es[i].pid = -1;
    self->pendencias_es[i].pid_espera_proc = -1;
  }
}

static int obter_posicao_livre_pendencias_es(so_t *self)
{
  for (int i = 0; i < MAX_PROCESSOS; i++)
  {
    if (self->pendencias_es[i].pid == -1)
    {
      return i;
    }
  }
  return -1;
}

/*
  param: Pode ser o id do dispositivo causador da pendência ou o pid do processo a ser esperado
  pid: pid do processo solicitante que causou a pendência
*/
static void adiciona_pendencia(so_t *self, int param, int pid, tipo_pendencia tipo_pendencia)
{
  int posicao = obter_posicao_livre_pendencias_es(self);

  if (posicao == -1)
  {
    console_printf(self->console, "Não há espaço para mais pendências."); // Isso nunca deve ocorrer
    return;
  }

  if (tipo_pendencia == ENTRADA || tipo_pendencia == SAIDA)
  {
    self->pendencias_es[posicao].pid = pid;
    self->pendencias_es[posicao].dispositivo = param;
  }
  else if (tipo_pendencia == ESPERA_PROC)
  {
    self->pendencias_es[posicao].pid = pid;
    self->pendencias_es[posicao].pid_espera_proc = param;
  }
}
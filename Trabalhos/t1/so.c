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

#define QUANTUM 50

#define ESCALONADOR_ROUND_ROBIN 0
#define ESCALONADOR_PRIO 1
#define ESCALONADOR ESCALONADOR_ROUND_ROBIN

typedef enum
{
  EXECUCAO,
  PRONTO,
  BLOQUEADO,
  QT_ESTADO_PROCESSO
} estado_processo;

/*
- int qt_processos_criados: número de processos criados
tempo total de execução
tempo total em que o sistema ficou ocioso (todos os processos bloqueados)
- int qt_interrupcoes[N_IRQ]: número de interrupções recebidas de cada tipo
- int qt_preempcoes[MAX_PROCESSOS]: número de preempções
tempo de retorno de cada processo (diferença entre data do término e da criação)
- int qt_preempcoes[MAX_PROCESSOS]: número de preempções de cada processo
número de vezes que cada processo entrou em cada estado (pronto, bloqueado, executando)
tempo total de cada processo em cada estado (pronto, bloqueado, executando)
tempo médio de resposta de cada processo (tempo médio em estado pronto)
*/
struct metricas_t 
{
  int qt_processos_criados;
  int qt_interrupcoes[N_IRQ];
  int qt_preempcoes[MAX_PROCESSOS];
  int qt_processos_estado[MAX_PROCESSOS][QT_ESTADO_PROCESSO];
  int tempo_execucao_processo[MAX_PROCESSOS];
  int tempo_total_processo_estado[MAX_PROCESSOS][QT_ESTADO_PROCESSO];
  int tempo_ocioso;
};

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
  float prio;
  int t_exec_inicio;
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

// Fila
struct Node {
    int pid;
    Node* next;
};

struct Fila {
    Node* front;
    Node* rear;
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

  /*
    escalonador circular preemptivo 
  */
  Fila fila;
  int quantum_counter;

  metricas_t metricas_t;
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
static int recupera_posicao_processo_por_pid(so_t *self, int pid);
static int recupera_posicao_processo_atual(so_t *self);
// static int recupera_posicao_primeiro_processo_pronto(so_t *self);
static int adiciona_processo_na_tabela(so_t *self, processo_t novo_processo);
static int recupera_posicao_livre_tabela_de_processos(so_t *self);
static void mata_processo(so_t *self, int posicao);
static int recupera_posicao_tabela_de_processos(so_t *self, int pid);
static void desbloqueia_processo(so_t *self, int pid);
static bool existe_processo(so_t *self, int pid);
static void altera_estado_processo(so_t *self, int i, estado_processo estado);
static int recupera_processo_maior_prio(so_t *self);

// função de tratamento de terminal/dispositivo
static int obter_terminal_por_pid(int pid);
static int obter_dipositivo_por_term(int term, int op);

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
static void so_escalona_prio(so_t *self);
static void so_despacha(so_t *self);

// escalonador preemptivo
static void atualiza_quantum_counter(so_t *self);
static void inicializa_quantum_counter(so_t *self);
static void print_fila(so_t *self);
static int dequeue_processo(Fila* fila);
static void enqueue_processo(so_t *self, Fila* fila, int pid); 
static Fila cria_fila();
static void calcula_prio(so_t *self, int pid);
static void inicializa_t_exec_inicio(so_t *self, int pid);

// funções para métricas
static void inicializa_metricas(so_t *self);
static void log_cria_processo(so_t *self);
static void log_preempcao(so_t *self, int i);
static void log_interrupcao(so_t *self, irq_t irq);
static void exibe_metricas(so_t *self);
static int calcula_total_de_preempcoes(metricas_t metricas);
static void log_processo_estado(so_t *self, int i, estado_processo estado);
static void log_ocioso(so_t *self);
static void log_tempo_execucao_processo(so_t *self, int i);
static void log_tempo_total_processo_estado(so_t *self, int i, estado_processo estado);

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
  self->fila = cria_fila();
  inicializa_quantum_counter(self);
  inicializa_tabela_processos(self);
  inicializa_pendencias_es(self);
  inicializa_metricas(self);
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
  if(ESCALONADOR == ESCALONADOR_ROUND_ROBIN) {
    so_escalona(self);
  } else if (ESCALONADOR == ESCALONADOR_PRIO) {
    so_escalona_prio(self);
  }
  // recupera o esÍtado do processo escolhido
  so_despacha(self);

  if(self->quantum_counter < 0) {
    return ERR_CPU_PARADA;
  }

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
  int processo_atual = recupera_posicao_processo_atual(self);
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
  int i = recupera_posicao_processo_por_pid(self, pid);
  log_tempo_execucao_processo(self,i);
  altera_estado_processo(self, i, PRONTO);
  log_tempo_total_processo_estado(self, i, PRONTO);
  enqueue_processo(self, &self->fila, pid);   
}

static void altera_estado_processo(so_t *self, int i, estado_processo estado) {
  self->tabela_processos[i].estado_processo = estado;
  if(estado == BLOQUEADO) {
    calcula_prio(self, self->tabela_processos[i].pid);
  }
  log_processo_estado(self, i, estado);
  log_tempo_total_processo_estado(self, i, estado);
}

static void so_escalona(so_t *self)
{
  // Define essa variável que pode receber o PID do estado anterior ou do próximo.
  // Caso não haja processo para escalonar, a variável mantém o valor que define que nenhum processo está em execução.
  int pid_processo_escalonado = NENHUM_PROCESSO_EM_EXECUCAO;

  print_fila(self);
  console_printf(self->console, "QUANTUM %d", self->quantum_counter);

  int i_processo_atual = recupera_posicao_processo_atual(self);

  if(i_processo_atual == -1) {
     //Obtem o primeiro processo da fila
    int pid = dequeue_processo(&self->fila);
    if(pid == -1) {
      console_printf(self->console, "Não há processos na fila");
    } else {
      //Escolhe o primeiro processo da fila para escalonar
      pid_processo_escalonado = pid;
      inicializa_quantum_counter(self);
    }
  } else { // existe um processo
    processo_t processo_atual = self->tabela_processos[i_processo_atual];

      if(self->quantum_counter == 0 || processo_atual.estado_processo == BLOQUEADO) { // O atualiza_quantum_counter descrementa o contador, então quando chegar em zero o processo deve ser preemptado
        if(processo_atual.estado_processo != BLOQUEADO) {
          //Adiciona o processo preemptado ao final da fila
          enqueue_processo(self, &self->fila, processo_atual.pid);  
        }
        
        //Obtem o primeiro processo da fila
        int pid = dequeue_processo(&self->fila);
        if(pid == -1) {
          console_printf(self->console, "Não há processos na fila");
        } else {
          //Escolhe o primeiro processo da fila para escalonar
          pid_processo_escalonado = pid;
          inicializa_quantum_counter(self);
        }
      } else {
        //Escalona o processo atual
        pid_processo_escalonado = self->pid_processo_em_execucao;
      }
  }

  // escolhe o próximo processo a executar, que passa a ser o processo
  //   corrente; pode continuar sendo o mesmo de antes ou não
  self->pid_processo_em_execucao = pid_processo_escalonado;
  console_printf(self->console, "Processo escalonado: %d", self->pid_processo_em_execucao);
}

static void so_escalona_prio(so_t *self)
{
  int pid_processo_escalonado = recupera_processo_maior_prio(self);

  if((self->pid_processo_em_execucao != pid_processo_escalonado) || self->quantum_counter == 0) {
    calcula_prio(self, self->pid_processo_em_execucao);
    inicializa_quantum_counter(self);
    inicializa_t_exec_inicio(self, pid_processo_escalonado);
  } 

  console_printf(self->console, "QUANTUM %d", self->quantum_counter);
  self->pid_processo_em_execucao = pid_processo_escalonado;
  console_printf(self->console, "Processo escalonado: %d", self->pid_processo_em_execucao);
}

static void so_despacha(so_t *self)
{
  // se não houver processo corrente, coloca ERR_CPU_PARADA em IRQ_END_erro
  if (self->pid_processo_em_execucao == NENHUM_PROCESSO_EM_EXECUCAO) {
    // Não existe processo para executar
    coloca_cpu_em_estado_parada(self);
  }
  else {
    // se houver processo corrente, coloca todo o estado desse processo em
    //   IRQ_END_*
    int processo_escalonado = recupera_posicao_processo_atual(self);
    mem_escreve(self->mem, IRQ_END_A, (self->tabela_processos[processo_escalonado].estado_cpu.A));
    mem_escreve(self->mem, IRQ_END_X, (self->tabela_processos[processo_escalonado].estado_cpu.X));
    mem_escreve(self->mem, IRQ_END_complemento, (self->tabela_processos[processo_escalonado].estado_cpu.complemento));
    mem_escreve(self->mem, IRQ_END_PC, (self->tabela_processos[processo_escalonado].estado_cpu.PC));
    mem_escreve(self->mem, IRQ_END_modo, (self->tabela_processos[processo_escalonado].estado_cpu.modo));
    mem_escreve(self->mem, IRQ_END_erro, (self->tabela_processos[processo_escalonado].estado_cpu.erro));
    // Define estado do processo para EXECUCAO
    altera_estado_processo(self, processo_escalonado, EXECUCAO);
    log_tempo_total_processo_estado(self, processo_escalonado, EXECUCAO);
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
  log_interrupcao(self, irq);
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

  // cria processo; escreve o endereço de carga no pc do descritor do processo; e muda o modo para usuário
  cria_processo(self, ender, -1);

  return ERR_OK;
}

static err_t so_trata_irq_err_cpu(so_t *self)
{
  // Ocorreu um erro interno na CPU
  // O erro está codificado em IRQ_END_erro
  // Em geral, causa a morte do processo que causou o erro

  int processo_atual = recupera_posicao_processo_atual(self);

  int err_int = self->tabela_processos[processo_atual].estado_cpu.erro;
  console_printf(self->console, "Lendo erro %d", err_int);
  err_t err = err_int;
  console_printf(self->console,
                 "SO: Tratando -- erro na CPU: %s", err_nome(err));

  mata_processo(self, self->pid_processo_em_execucao); // Estou tratando todos os erros com a morte do processo. Devo tratar de outra forma?

  return ERR_OK;
}

// -1 - inicializa com agora
// agora - final 

static err_t so_trata_irq_relogio(so_t *self)
{
  // ocorreu uma interrupção do relógio
  // rearma o interruptor do relógio e reinicializa o timer para a próxima interrupção
  rel_escr(self->relogio, 3, 0); // desliga o sinalizador de interrupção
  rel_escr(self->relogio, 2, INTERVALO_INTERRUPCAO);
  int i = recupera_posicao_processo_atual(self);
  self->tabela_processos[i].t_exec_inicio++;
  atualiza_quantum_counter(self);
  log_ocioso(self);
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
  int posicao = recupera_posicao_processo_atual(self);
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
  int i_processo = recupera_posicao_processo_atual(self);
  int term = obter_terminal_por_pid(self->tabela_processos[i_processo].pid);

  int estado;
  int id_el = obter_dipositivo_por_term(term, 1);
  term_le(self->console, id_el, &estado);
  // Dispositivo disponível - Atende solicitação
  if (estado != 0){
    atende_leitura(self, term, self->tabela_processos[i_processo].pid);
  }
  else { // Dispositivo indisponível - Bloqueia processo
    altera_estado_processo(self, i_processo, BLOQUEADO);
    log_tempo_total_processo_estado(self, i_processo, BLOQUEADO);
    adiciona_pendencia(self, id_el, self->tabela_processos[i_processo].pid, ENTRADA);
  }
}

static void so_chamada_escr(so_t *self)
{
  int i_processo = recupera_posicao_processo_atual(self);
  int term = obter_terminal_por_pid(self->tabela_processos[i_processo].pid);

  int estado;
  int id_ee = obter_dipositivo_por_term(term, 3);
  term_le(self->console, id_ee, &estado);
  // Dispositivo disponível - Atende solicitação
  if (estado != 0) {
    atende_escrita(self, term);
  } else { // Dispositivo indisponível - Bloqueia processo
    altera_estado_processo(self, i_processo, BLOQUEADO);
    log_tempo_total_processo_estado(self, i_processo, BLOQUEADO);
    adiciona_pendencia(self, id_ee, self->tabela_processos[i_processo].pid, SAIDA);
  }
}

static void so_chamada_espera_proc(so_t *self)
{
  int i_processo_solicitante = recupera_posicao_processo_atual(self);
  int pid_espera_proc = self->tabela_processos[i_processo_solicitante].estado_cpu.X;
  bool existe = existe_processo(self, pid_espera_proc);

  if (existe) {
    adiciona_pendencia(self, pid_espera_proc, self->tabela_processos[i_processo_solicitante].pid, ESPERA_PROC);
    altera_estado_processo(self, i_processo_solicitante, BLOQUEADO);
    log_tempo_total_processo_estado(self, i_processo_solicitante, BLOQUEADO);
    self->tabela_processos[i_processo_solicitante].estado_cpu.A = 0; //Retorna sucesso
  }
  else {
    self->tabela_processos[i_processo_solicitante].estado_cpu.A = -1; //Retorna erro
  }
}

/*
  Faz a leitura de um dispositivo de E/S.
  Essa função só pode ser chamada antes de verificar o estado do dispositivo. Ele deve estar pronto para leitura.
*/
static void atende_leitura(so_t *self, int term, int pid)
{
  int i_processo = recupera_posicao_processo_por_pid(self, pid);
  int dado;
  int id_dl = obter_dipositivo_por_term(term, 0);
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
  int id_de = obter_dipositivo_por_term(term, 2);
  term_escr(self->console, id_de, dado);
  mem_escreve(self->mem, IRQ_END_A, 0);
}

static void so_chamada_cria_proc(so_t *self)
{
  // Recupera o processo atual. É ele que gerou uma chamada de sistema para criação de outro processo.
  int processo_pai = recupera_posicao_processo_por_pid(self, self->pid_processo_em_execucao);

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
  int posicao_processo_chamador = recupera_posicao_processo_atual(self);
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
    self->tabela_processos[i].pid = -1;
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
  novo_processo.prio = 0.5;
  novo_processo.t_exec_inicio = 0;
  adiciona_processo_na_tabela(self, novo_processo);

  //Adiciona o processo no fim da fila
  enqueue_processo(self, &self->fila, novo_processo.pid);
  log_cria_processo(self);
  return novo_processo.pid;
}

static int geraPid(so_t *self)
{
  self->count_processos++;
  return self->count_processos;
}

// Recupera um processo com base em algum PID
static int recupera_posicao_processo_por_pid(so_t *self, int pid)
{
  for (int i = 0; i < MAX_PROCESSOS; i++) {
    if (self->tabela_processos[i].pid == pid && !self->tabela_processos[i].livre) {
      return i;
    }
  }

  return -1;
}

// Recupera o processo atual com base no pid_processo_em_execucao
static int recupera_posicao_processo_atual(so_t *self)
{
  return recupera_posicao_processo_por_pid(self, self->pid_processo_em_execucao);
}

// Recupera o primeiro processo que estiver com o estado PRONTO
// static int recupera_posicao_primeiro_processo_pronto(so_t *self)
// {
//   for (int i = 0; i < MAX_PROCESSOS; i++) {
//     if (self->tabela_processos[i].estado_processo == PRONTO && !self->tabela_processos[i].livre) {
//       return i;
//     }
//   }

//   return -1;
// }

static int adiciona_processo_na_tabela(so_t *self, processo_t novo_processo)
{
  int posicao = recupera_posicao_livre_tabela_de_processos(self);
  if (posicao != -1) {
    self->tabela_processos[posicao] = novo_processo;
    log_tempo_execucao_processo(self,posicao);
    log_processo_estado(self, posicao, PRONTO);
    log_tempo_total_processo_estado(self, posicao, PRONTO);
  }
  else {
    console_printf(self->console, "Tabela cheia"); // Tabela de processos cheia. O que fazer?
  }

  return posicao;
}

// Recupera o primeiro processo que estiver com o estado PRONTO
static int recupera_posicao_livre_tabela_de_processos(so_t *self)
{
  for (int i = 0; i < MAX_PROCESSOS; i++) {
    if (self->tabela_processos[i].livre) {
      return i;
    }
  }
  return -1;
}

// Recupera posição do processo com base no PID
static int recupera_posicao_tabela_de_processos(so_t *self, int pid)
{
  for (int i = 0; i < MAX_PROCESSOS; i++) {
    if (self->tabela_processos[i].pid == pid && !self->tabela_processos[i].livre) {
      return i;
    }
  }
  return -1;
}

static int recupera_processo_maior_prio(so_t *self) {
  float prio = -1;
  int pid_processo_maior_prio = -1;

  for(int i=0; i<MAX_PROCESSOS; i++) {
    processo_t processo = self->tabela_processos[i];
    if(!processo.livre && processo.estado_processo != BLOQUEADO) {
      if(processo.prio > prio) {
        prio = processo.prio;
        pid_processo_maior_prio = processo.pid;
      }
    }
  }
  return pid_processo_maior_prio;
}

static void mata_processo(so_t *self, int pid)
{
  int i = recupera_posicao_tabela_de_processos(self, pid);
  if (i != -1) {
    self->tabela_processos[i].livre = true;
    self->tabela_processos[i].estado_processo = BLOQUEADO;
    self->quantum_counter = 0;

    if(pid == 1) {
      exibe_metricas(self);
    }
  } else {
    console_printf(self->console, "Processo não encontrado");
  }
}

static bool existe_processo(so_t *self, int pid)
{
  for (int i = 0; i < MAX_PROCESSOS; i++) {
    if (self->tabela_processos[i].pid == pid && !self->tabela_processos[i].livre) {
      return true;
    }
  }
  return false;
}

static int obter_terminal_por_pid(int pid)
{
  return ((pid - 1) % 4);
}

static int obter_dipositivo_por_term(int term, int op)
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
  for (int i = 0; i < MAX_PROCESSOS; i++) {
    self->pendencias_es[i].dispositivo = -1;
    self->pendencias_es[i].pid = -1;
    self->pendencias_es[i].pid_espera_proc = -1;
  }
}

static int obter_posicao_livre_pendencias_es(so_t *self)
{
  for (int i = 0; i < MAX_PROCESSOS; i++) {
    if (self->pendencias_es[i].pid == -1) {
      return i;
    }
  }
  return -1; //Isso nunca deve ocorrer
}

/*
  param: Pode ser o id do dispositivo causador da pendência ou o pid do processo a ser esperado
  pid: pid do processo solicitante que causou a pendência
*/
static void adiciona_pendencia(so_t *self, int param, int pid, tipo_pendencia tipo_pendencia)
{
  int posicao = obter_posicao_livre_pendencias_es(self);

  if (posicao == -1) {
    console_printf(self->console, "Não há espaço para mais pendências."); // Isso nunca deve ocorrer
    return;
  }

  if (tipo_pendencia == ENTRADA || tipo_pendencia == SAIDA) {
    self->pendencias_es[posicao].pid = pid;
    self->pendencias_es[posicao].dispositivo = param;
  } else if (tipo_pendencia == ESPERA_PROC) {
    self->pendencias_es[posicao].pid = pid;
    self->pendencias_es[posicao].pid_espera_proc = param;
  }

  self->pendencias_es[posicao].tipo_pendencia = tipo_pendencia;
}

// ESCALONADOR CIRCULAR PREEPTIVO

// FILA

static Fila cria_fila() {
    Fila fila;
    fila.front = fila.rear = NULL;
    return fila;
}

static void enqueue_processo(so_t *self, Fila* fila, int pid) {
  int i = recupera_posicao_processo_por_pid(self, pid);
    log_preempcao(self, i);

    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->pid = pid;
    new_node->next = NULL;

    if (fila->rear == NULL) {
        fila->front = fila->rear = new_node;
        return;
    }

    fila->rear->next = new_node;
    fila->rear = new_node;
}

static int dequeue_processo(Fila* fila) {
    if (fila->front == NULL) {
        return -1;
    }

    Node* temp = fila->front;
    int pid = temp->pid;
    fila->front = fila->front->next;

    if (fila->front == NULL)
        fila->rear = NULL;

    free(temp);
    return pid;
}

static void print_fila(so_t *self) {
    Node* temp = self->fila.front;
    if (temp == NULL) {
        console_printf(self->console, "Queue is empty\n");
        return;
    }

    while (temp != NULL) {
        console_printf(self->console, "%d ", temp->pid);
        temp = temp->next;
    }
}

// ESCALONADOR
static void atualiza_quantum_counter(so_t *self) {
  self->quantum_counter--;
}

static void inicializa_quantum_counter(so_t *self) {
  self->quantum_counter = QUANTUM;
}

static void inicializa_t_exec_inicio(so_t *self, int pid) {
  int i = recupera_posicao_processo_por_pid(self, pid);
  self->tabela_processos[i].t_exec_inicio = rel_agora(self->relogio);
}

static void calcula_prio(so_t *self, int pid) {
  int i = recupera_posicao_processo_por_pid(self, pid);
  if(i != -1) {
    int t_exec = rel_agora(self->relogio) - self->tabela_processos[i].t_exec_inicio;
    self->tabela_processos[i].prio = self->tabela_processos[i].prio + (t_exec / (self->quantum_counter * INTERVALO_INTERRUPCAO));
  }
}

//MÉTRICAS
static void inicializa_metricas(so_t *self) {
  self->metricas_t.qt_processos_criados = 0;
  for(int i=0; i<MAX_PROCESSOS; i++) {
    self->metricas_t.qt_preempcoes[i] = 0;
    self->metricas_t.tempo_ocioso = 0;
    self->metricas_t.tempo_execucao_processo[i] = 0;
    for(int j=0; j<3; j++) {
      self->metricas_t.qt_processos_estado[i][j] = 0;
      self->metricas_t.tempo_total_processo_estado[i][j] = 0;
    }
  }
}

static void log_cria_processo(so_t *self) {
  self->metricas_t.qt_processos_criados++;
}

static void log_preempcao(so_t *self, int i) {
  self->metricas_t.qt_preempcoes[i]++;
}

static void log_interrupcao(so_t *self, irq_t irq) {
  self->metricas_t.qt_interrupcoes[irq]++;
}

static void log_processo_estado(so_t *self, int i, estado_processo estado) {
  self->metricas_t.qt_processos_estado[i][estado]++;
}

static void log_ocioso(so_t *self){
  self->metricas_t.tempo_ocioso++;
}

static void log_tempo_total_processo_estado(so_t *self, int i, estado_processo estado) {
  self->metricas_t.tempo_total_processo_estado[i][estado] = rel_agora(self->relogio)  - self->tabela_processos[i].t_exec_inicio * INTERVALO_INTERRUPCAO;
}

static void log_tempo_execucao_processo(so_t *self, int i){
  self->metricas_t.tempo_execucao_processo[i] = rel_agora(self->relogio)  - self->tabela_processos[i].t_exec_inicio * INTERVALO_INTERRUPCAO;
}

// relogio - instrucoes executadas
// t_exec - quantidade de interrupcoes de relogio;

static void exibe_metricas(so_t *self) {
  console_printf(self->console, "\n");
  console_printf(self->console, "\n");
  console_printf(self->console, "*--------------------------------------- RELATÓRIO DE SISTEMA ---------------------------------------*");
  console_printf(self->console, "\n");
  console_printf(self->console, "         - Quantidade de processos criados: %d", self->metricas_t.qt_processos_criados);
  console_printf(self->console, "         - Quantidade de preempções totais: %d", calcula_total_de_preempcoes(self->metricas_t));
  console_printf(self->console, "         - Tempo ocioso: %d", self->metricas_t.tempo_ocioso);
  console_printf(self->console, "         - Quantidade de preempções por processo:");
  for(int i=0; i<MAX_PROCESSOS; i++) {
      processo_t processo = self->tabela_processos[i];
      if(processo.pid != -1) {
        console_printf(self->console, "             - PID %d: %d preempções", processo.pid, self->metricas_t.qt_preempcoes[i]);
      }
  }

  console_printf(self->console, "         - Tempo de retorno de cada processo:");
  for(int i=0; i<MAX_PROCESSOS; i++) {
      processo_t processo = self->tabela_processos[i];
      if(processo.pid != -1) {
        console_printf(self->console, "             - PID %d: %d por interrupção", processo.pid, self->metricas_t.tempo_execucao_processo[i]);
      }
  }

  console_printf(self->console, "         - Tempo total de cada processo em cada estado:");
  for(int i=0; i<MAX_PROCESSOS; i++) {
      processo_t processo = self->tabela_processos[i];
      if(processo.pid != -1) {
        console_printf(self->console, "             - PID %d: PRONTO: %d; EXECUCAO: %d, BLOQUEADO: %d", processo.pid, self->metricas_t.tempo_total_processo_estado[i][PRONTO], self->metricas_t.tempo_total_processo_estado[i][EXECUCAO], self->metricas_t.tempo_total_processo_estado[i][BLOQUEADO]);
      }
  }

  console_printf(self->console, "         - Quantidade de estados por processo:");
  for(int i=0; i<MAX_PROCESSOS; i++) {
      processo_t processo = self->tabela_processos[i];
      if(processo.pid != -1) {
        console_printf(self->console, "             - PID %d: PRONTO: %d; EXECUCAO: %d, BLOQUEADO: %d", processo.pid, self->metricas_t.qt_processos_estado[i][PRONTO], self->metricas_t.qt_processos_estado[i][EXECUCAO], self->metricas_t.qt_processos_estado[i][BLOQUEADO]);
      }
  }
  console_printf(self->console, "         - Quantidade de interrupcões: ");
  console_printf(self->console, "             - IRQ_RESET: %d", self->metricas_t.qt_interrupcoes[IRQ_RESET]);
  console_printf(self->console, "             - IRQ_ERR_CPU: %d", self->metricas_t.qt_interrupcoes[IRQ_ERR_CPU]);
  console_printf(self->console, "             - IRQ_SISTEMA: %d", self->metricas_t.qt_interrupcoes[IRQ_SISTEMA]);
  console_printf(self->console, "             - IRQ_RELOGIO: %d", self->metricas_t.qt_interrupcoes[IRQ_RELOGIO]);
  console_printf(self->console, "             - IRQ_TECLADO: %d", self->metricas_t.qt_interrupcoes[IRQ_TECLADO]);
  console_printf(self->console, "             - IRQ_TELA: %d", self->metricas_t.qt_interrupcoes[IRQ_TELA]);
  console_printf(self->console, "\n*---------------------------------------------------------------------------------------------------*");
  console_printf(self->console, "\n");
  console_printf(self->console, "\n");
}

static int calcula_total_de_preempcoes(metricas_t metricas) {
  int total = 0;
  for(int i=0 ; i<MAX_PROCESSOS; i++) {
    total += metricas.qt_preempcoes[i];
  }
  return total;
}


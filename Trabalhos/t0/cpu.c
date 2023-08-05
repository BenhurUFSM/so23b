#include "cpu.h"
#include "instrucao.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// uma CPU tem estado, memória, controlador de ES
struct cpu_t {
  // registradores
  int PC;
  int A;
  int X;
  // estado interno da CPU
  err_t erro;
  int complemento;
  // acesso a dispositivos externos
  mem_t *mem;
  es_t *es;
};

cpu_t *cpu_cria(mem_t *mem, es_t *es)
{
  cpu_t *self;
  self = malloc(sizeof(*self));
  if (self != NULL) {
    self->PC = 0;
    self->A = 0;
    self->X = 0;
    self->erro = ERR_OK;
    self->complemento = 0;
    self->mem = mem;
    self->es = es;
  }
  return self;
}

void cpu_destroi(cpu_t *self)
{
  // eu nao criei memória nem es; quem criou que destrua!
  free(self);
}

char *cpu_descricao(cpu_t *self)
{
  static char descr[100]; 
  // imprime registradores, opcode, instrução
  int opcode = -1;
  mem_le(self->mem, self->PC, &opcode);
  sprintf(descr, "PC=%04d A=%06d X=%06d %02d %s",
                 self->PC, self->A, self->X, opcode, instrucao_nome(opcode));
  // imprime argumento da instrução, se houver
  if (instrucao_num_args(opcode) > 0) {
    char aux[40];
    int A1;
    mem_le(self->mem, self->PC + 1, &A1);
    sprintf(aux, " %d", A1);
    strcat(descr, aux);
  }
  // imprime estado de erro da CPU, se for o caso
  if (self->erro != ERR_OK) {
    char aux[40];
    sprintf(aux, " E=%d(%d) %s",
                 self->erro, self->complemento, err_nome(self->erro));
    strcat(descr, aux);
  }
  return descr;
}


// ---------------------------------------------------------------------
// funções auxiliares para usar durante a execução das instruções
// alteram o estado da CPU caso ocorra erro

// lê um valor da memória
static bool pega_mem(cpu_t *self, int endereco, int *pval)
{
  self->erro = mem_le(self->mem, endereco, pval);
  if (self->erro == ERR_OK) return true;
  self->complemento = endereco;
  return false;
}

// lê o opcode da instrução no PC
static bool pega_opcode(cpu_t *self, int *popc)
{
  return pega_mem(self, self->PC, popc);
}

// lê o argumento 1 da instrução no PC
static bool pega_A1(cpu_t *self, int *pA1)
{
  return pega_mem(self, self->PC + 1, pA1);
}

// escreve um valor na memória
static bool poe_mem(cpu_t *self, int endereco, int val)
{
  self->erro = mem_escreve(self->mem, endereco, val);
  if (self->erro == ERR_OK) return true;
  self->complemento = endereco;
  return false;
}

// lê um valor da E/S
static bool pega_es(cpu_t *self, int dispositivo, int *pval)
{
  self->erro = es_le(self->es, dispositivo, pval);
  if (self->erro == ERR_OK) return true;
  self->complemento = dispositivo;
  return false;
}

// escreve um valor na E/S
static bool poe_es(cpu_t *self, int dispositivo, int val)
{
  self->erro = es_escreve(self->es, dispositivo, val);
  if (self->erro == ERR_OK) return true;
  self->complemento = dispositivo;
  return false;
}

// ---------------------------------------------------------------------
// funções auxiliares para implementação de cada instrução

static void op_NOP(cpu_t *self) // não faz nada
{
  self->PC += 1;
}

static void op_PARA(cpu_t *self) // para a CPU
{
  self->erro = ERR_CPU_PARADA;
}

static void op_CARGI(cpu_t *self) // carrega imediato
{
  int A1;
  if (pega_A1(self, &A1)) {
    self->A = A1;
    self->PC += 2;
  }
}

static void op_CARGM(cpu_t *self) // carrega da memória
{
  int A1, mA1;
  if (pega_A1(self, &A1) && pega_mem(self, A1, &mA1)) {
    self->A = mA1;
    self->PC += 2;
  }
}

static void op_CARGX(cpu_t *self) // carrega indexado
{
  int A1, mA1mX;
  int X = self->X;
  if (pega_A1(self, &A1) && pega_mem(self, A1 + X, &mA1mX)) {
    self->A = mA1mX;
    self->PC += 2;
  }
}

static void op_ARMM(cpu_t *self) // armazena na memória
{
  int A1;
  if (pega_A1(self, &A1) && poe_mem(self, A1, self->A)) {
    self->PC += 2;
  }
}

static void op_ARMX(cpu_t *self) // armazena indexado
{
  int A1;
  int X = self->X;
  if (pega_A1(self, &A1) && poe_mem(self, A1 + X, self->A)) {
    self->PC += 2;
  }
}

static void op_TRAX(cpu_t *self) // troca A com X
{
  int A = self->A;
  int X = self->X;
  self->X = A;
  self->A = X;
  self->PC += 1;
}

static void op_CPXA(cpu_t *self) // copia X para A
{
  self->A = self->X;
  self->PC += 1;
}

static void op_INCX(cpu_t *self) // incrementa X
{
  self->X += 1;
  self->PC += 1;
}

static void op_SOMA(cpu_t *self) // soma
{
  int A1, mA1;
  if (pega_A1(self, &A1) && pega_mem(self, A1, &mA1)) {
    self->A += mA1;
    self->PC += 2;
  }
}

static void op_SUB(cpu_t *self) // subtração
{
  int A1, mA1;
  if (pega_A1(self, &A1) && pega_mem(self, A1, &mA1)) {
    self->A -= mA1;
    self->PC += 2;
  }
}

static void op_MULT(cpu_t *self) // multiplicação
{
  int A1, mA1;
  if (pega_A1(self, &A1) && pega_mem(self, A1, &mA1)) {
    self->A *= mA1;
    self->PC += 2;
  }
}

static void op_DIV(cpu_t *self) // divisão
{
  int A1, mA1;
  if (pega_A1(self, &A1) && pega_mem(self, A1, &mA1)) {
    self->A /= mA1;
    self->PC += 2;
  }
}

static void op_RESTO(cpu_t *self) // resto
{
  int A1, mA1;
  if (pega_A1(self, &A1) && pega_mem(self, A1, &mA1)) {
    self->A %= mA1;
    self->PC += 2;
  }
}

static void op_NEG(cpu_t *self) // inverte sinal
{
  self->A = -self->A;
  self->PC += 1;
}

static void op_DESV(cpu_t *self) // desvio incondicional
{
  int A1;
  if (pega_A1(self, &A1)) {
    self->PC = A1;
  }
}

static void op_DESVZ(cpu_t *self) // desvio condicional
{
  if (self->A == 0) {
    op_DESV(self);
  } else {
    self->PC += 2;
  }
}

static void op_DESVNZ(cpu_t *self) // desvio condicional
{
  if (self->A != 0) {
    op_DESV(self);
  } else {
    self->PC += 2;
  }
}

static void op_DESVN(cpu_t *self) // desvio condicional
{
  if (self->A < 0) {
    op_DESV(self);
  } else {
    self->PC += 2;
  }
}

static void op_DESVP(cpu_t *self) // desvio condicional
{
  if (self->A > 0) {
    op_DESV(self);
  } else {
    self->PC += 2;
  }
}

static void op_CHAMA(cpu_t *self) // chamada de subrotina
{
  int A1;
  if (pega_A1(self, &A1) && poe_mem(self, A1, self->PC + 2)) {
    self->PC = A1 + 1;
  }
}

static void op_RET(cpu_t *self) // retorno de subrotina
{
  int A1, mA1;
  if (pega_A1(self, &A1) && pega_mem(self, A1, &mA1)) {
    self->PC = mA1;
  }
}

static void op_LE(cpu_t *self) // leitura de E/S
{
  int A1, dado;
  if (pega_A1(self, &A1) && pega_es(self, A1, &dado)) {
    self->A = dado;
    self->PC += 2;
  }
}

static void op_ESCR(cpu_t *self) // escrita de E/S
{
  int A1;
  if (pega_A1(self, &A1) && poe_es(self, A1, self->A)) {
    self->PC += 2;
  }
}


err_t cpu_executa_1(cpu_t *self)
{
  // não executa se CPU já estiver em erro
  if (self->erro != ERR_OK) return self->erro;

  int opcode;
  if (!pega_opcode(self, &opcode)) return self->erro;

  switch (opcode) {
    case NOP:    op_NOP(self);    break;
    case PARA:   op_PARA(self);   break;
    case CARGI:  op_CARGI(self);  break;
    case CARGM:  op_CARGM(self);  break;
    case CARGX:  op_CARGX(self);  break;
    case ARMM:   op_ARMM(self);   break;
    case ARMX:   op_ARMX(self);   break;
    case TRAX:   op_TRAX(self);   break;
    case CPXA:   op_CPXA(self);   break;
    case INCX:   op_INCX(self);   break;
    case SOMA:   op_SOMA(self);   break;
    case SUB:    op_SUB(self);    break;
    case MULT:   op_MULT(self);   break;
    case DIV:    op_DIV(self);    break;
    case RESTO:  op_RESTO(self);  break;
    case NEG:    op_NEG(self);    break;
    case DESV:   op_DESV(self);   break;
    case DESVZ:  op_DESVZ(self);  break;
    case DESVNZ: op_DESVNZ(self); break;
    case DESVN:  op_DESVN(self);  break;
    case DESVP:  op_DESVP(self);  break;
    case CHAMA:  op_CHAMA(self);  break;
    case RET:    op_RET(self);    break;
    case LE:     op_LE(self);     break;
    case ESCR:   op_ESCR(self);   break;
    default:     self->erro = ERR_INSTR_INV;
  }

  return self->erro;
}

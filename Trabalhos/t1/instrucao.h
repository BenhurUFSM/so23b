#ifndef INSTRUCAO_H
#define INSTRUCAO_H

// As instruções implementadas pelo simumador e as pseudo instruções do
//   montador
// As pseudo-instruções, processadas pelo montador e que não geram código são:
//   VALOR  - inicaliza a próxima posição de memória com o valor do argumento
//   STRING - inicializa as próximas posições com o valor de cada caractere
//            entre aspas (simples ou duplas), mais um valor 0 no final
//   ESPACO - reserva tantas palavras de espaço nas próximas posições da
//            memória (corresponde a tantos "VALOR 0")
//   DEFINE - define um valor para um símbolo (obrigatoriamente tem que ter
//            um label, que é definido com o valor do argumento e não com a
//            posição atual da memória)

typedef enum {
  // instruções normais
  // opcode     #arg  descrição
  NOP    =  0, // 1   não faz nada
  PARA   =  1, // 1   para a CPU             modo=ERR_CPU_PARADA
  CARGI  =  2, // 2   carrega imediato       A=A1
  CARGM  =  3, // 2   carrega da memória     A=mem[A1]
  CARGX  =  4, // 2   carrega indexado       A=mem[A1+X]
  ARMM   =  5, // 2   armazena na memória    mem[A1]=A
  ARMX   =  6, // 2   armazena indexado      mem[A1+X]=A
  TRAX   =  7, // 1   troca A com X          X<->A
  CPXA   =  8, // 1   copia X para A         A=X
  INCX   =  9, // 1   incrementa X           X++
  SOMA   = 10, // 2   soma                   A+=mem[A1]
  SUB    = 11, // 2   subtrai                A-=mem[A1]
  MULT   = 12, // 2   multiplica             A*=mem[A1]
  DIV    = 13, // 2   divide                 A/=mem[A1]
  RESTO  = 14, // 2   calcula o resto        A%=mem[A1]
  NEG    = 15, // 1   inverte o sinal        A=-A
  DESV   = 16, // 2   desvio                 PC=A1
  DESVZ  = 17, // 2   desvio condicional     se A for 0, PC=A1
  DESVNZ = 18, // 2   desvio condicional     se A não for 0, PC=A1
  DESVN  = 19, // 2   desvio condicional     se A < 0, PC=A1
  DESVP  = 20, // 2   desvio condicional     se A > 0, PC=A1
  CHAMA  = 21, // 2   chama subrotina        mem[A1]=PC+2; PC=A1+1
  RET    = 22, // 2   retorna de subrotina   PC=mem[A1]
  LE     = 23, // 2   leitura de E/S         A=es[A1]
  ESCR   = 24, // 2   escrita de E/S         es[A1]=A
  RETI   = 25, // 1   retorno de interrupção restaura estado da CPU
  CHAMAC = 26, // 1   chama função C         simula código compilado
  CHAMAS = 27, // 1   chama sistema          causa interrupção IRQ_SISTEMA
  // pseudo-instruções
  VALOR,
  STRING,
  ESPACO,
  DEFINE,
  N_OPCODE
} opcode_t;

// retorna o opcode do nome
opcode_t instrucao_opcode(char *nome);

// retorna o nome do opcode
char *instrucao_nome(int opcode);

// retorna o número de argumentos do opcode
int instrucao_num_args(int opcode);

#endif // INSTRUCAO_H

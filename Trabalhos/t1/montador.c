// montador para o processador de SO

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "instrucao.h"
// auxiliares

// aborta o programa com uma mensagem de erro
void erro_brabo(char *msg)
{
  fprintf(stderr, "ERRO FATAL: %s\n", msg);
  exit(1);
}

// retorna true se tem um número na string s (e retorna o número também)
bool tem_numero(char *s, int *num)
{
  if (isdigit(*s) || (*s == '-' && isdigit(*(s+1)))) {
    *num = atoi(s);
    return true;
  }
  if ((s[0] == '\'' || s[0] == '"') && s[2] == '\0') {
    *num = s[1];
    return true;
  }
  return false;
}

// memória de saída

// representa a memória do programa -- a saída do montador é colocada aqui

#define MEM_TAM 10000    // aumentar para programas maiores
int mem[MEM_TAM];
int mem_pos = 0;        // próxima posição livre da memória
int mem_min = -1;       // menor endereço preenchido
int mem_max = -1;       // maior endereço preenchido

char *nome_fonte;   // nome do arquivo fonte a montar

// coloca um valor no final da memória
void mem_insere(int val)
{
  if (mem_pos >= MEM_TAM-1) {
    erro_brabo("programa muito grande! Aumente MEM_TAM no montador.");
  }
  if (mem_min == -1 || mem_pos < mem_min) mem_min = mem_pos;
  if (mem_max == -1 || mem_pos > mem_max) mem_max = mem_pos;
  mem[mem_pos++] = val;
}

// altera o valor em uma posição já ocupada da memória
void mem_altera(int pos, int val)
{
  if (pos < mem_min || pos > mem_max) {
    erro_brabo("erro interno, alteração de região não inicializada");
  }
  mem[pos] = val;
}

// imprime o conteúdo da memória
void mem_imprime(void)
{
  printf("MAQ %d %d\n", mem_max - mem_min + 1, mem_min);
  for (int i = mem_min; i <= mem_max; i+=10) {
    printf("[%4d] =", i);
    for (int j = i; j < i+10 && j <= mem_max; j++) {
      printf(" %d,", mem[j]);
    }
    printf("\n");
  }
}

// simbolos

// tabela com os símbolos (labels) já definidos pelo programa, e o valor (endereço) deles

#define SIMB_TAM 1000
struct {
  char *nome;
  int valor;
} simbolo[SIMB_TAM];
int simb_num;             // número d símbolos na tabela

// retorna o valor de um símbolo, ou -1 se não existir na tabela
int simb_valor(char *nome)
{
  for (int i=0; i<simb_num; i++) {
    if (strcmp(nome, simbolo[i].nome) == 0) {
      return simbolo[i].valor;
    }
  }
  return -1;
}

// insere um novo símbolo na tabela
void simb_novo(char *nome, int valor)
{
  if (nome == NULL) return;
  if (simb_valor(nome) != -1) {
    fprintf(stderr, "ERRO: redefinicao do simbolo '%s'\n", nome);
    return;
  }
  if (simb_num >= SIMB_TAM) {
    erro_brabo("Excesso de símbolos. Aumente SIMB_TAM no montador.");
  }
  simbolo[simb_num].nome = strdup(nome);
  simbolo[simb_num].valor = valor;
  simb_num++;
}


// referências

// tabela com referências a símbolos
//   contém a linha e o endereço correspondente onde o símbolo foi referenciado

#define REF_TAM 1000
struct {
  char *nome;
  int linha;
  int endereco;
} ref[REF_TAM];
int ref_num;      // numero de referências criadas

// insere uma nova referência na tabela
void ref_nova(char *nome, int linha, int endereco)
{
  if (nome == NULL) return;
  if (ref_num >= REF_TAM) {
    erro_brabo("excesso de referências. Aumente REF_TAM no montador.");
  }
  ref[ref_num].nome = strdup(nome);
  ref[ref_num].linha = linha;
  ref[ref_num].endereco = endereco;
  ref_num++;
}

// resolve as referências -- para cada referência, coloca o valor do símbolo
//   no endereço onde ele é referenciado
void ref_resolve(void)
{
  for (int i=0; i<ref_num; i++) {
    int valor = simb_valor(ref[i].nome);
    if (valor == -1) {
      fprintf(stderr, 
              "ERRO: simbolo '%s' referenciado na linha %d não foi definido\n",
              ref[i].nome, ref[i].linha);
    }
    mem_altera(ref[i].endereco, valor);
  }
}



// montagem

// realiza a montagem de uma instrução (gera o código para ela na memória),
//   tendo opcode da instrução e o argumento
void monta_instrucao(int linha, int opcode, char *arg)
{
  int argn;  // para conter o valor numérico do argumento
  int num_args = instrucao_num_args(opcode);
  
  // trata pseudo-opcodes antes
  if (opcode == ESPACO) {
    if (!tem_numero(arg, &argn)) {
      argn = simb_valor(arg);
    }
    if (argn < 1) {
      fprintf(stderr, "ERRO: linha %d 'ESPACO' deve ter valor positivo\n",
              linha);
      return;
    }
    for (int i = 0; i < argn; i++) {
      mem_insere(0);
    }
    return;
  } else if (opcode == VALOR) {
    // nao faz nada, vai inserir o valor definido em arg
  } else if (opcode == STRING) {
    char c;
    do {
      c = *++arg;
      mem_insere(c);
    } while(c != '\0');
    return;
  } else {
    // instrução real, coloca o opcode da instrução na memória
    mem_insere(opcode);
  }
  if (num_args == 0) {
    return;
  }
  if (tem_numero(arg, &argn)) {
    mem_insere(argn);
  } else {
    // não é número, põe um 0 e insere uma referência para alterar depois
    ref_nova(arg, linha, mem_pos);
    mem_insere(0);
  }
}

// monta uma linha "label DEFINE arg", define o símbolo 'label' com valor 'arg'
void monta_define(int linha, char *label, char *arg)
{
  int argn;  // para conter o valor numérico do argumento
  if (label == NULL) {
    fprintf(stderr, "ERRO: linha %d: 'DEFINE' exige um label\n", linha);
  } else if (!tem_numero(arg, &argn)) {
    fprintf(stderr, "ERRO: linha %d 'DEFINE' exige valor numérico\n", linha);
  } else {
    // tudo OK, define o símbolo
    simb_novo(label, argn);
  }
}

// monta uma linha "label instrucao arg"
void monta_linha(int linha, char *label, char *instrucao, char *arg)
{
  int opcode = instrucao_opcode(instrucao);
  // pseudo-instrução DEFINE tem que ser tratada antes, porque não pode
  //   definir o label de forma normal
  if (opcode == DEFINE) {
    monta_define(linha, label, arg);
    return;
  }
  
  // cria símbolo correspondente ao label, se for o caso
  if (label != NULL) {
    simb_novo(label, mem_pos);
  }
  
  // verifica a existência de instrução e número correto de argumentos
  if (instrucao == NULL) return;
  if (opcode == -1) {
    fprintf(stderr, "ERRO: linha %d: instrucao '%s' desconhecida\n",
                    linha, instrucao);
    return;
  }
  int num_args = instrucao_num_args(opcode);
  if (num_args == 0 && arg != NULL) {
    fprintf(stderr, "ERRO: linha %d: instrucao '%s' não tem argumento\n",
                    linha, instrucao);
    return;
  }
  if (num_args == 1 && arg == NULL) {
    fprintf(stderr, "ERRO: linha %d: instrucao '%s' necessita argumento\n",
                    linha, instrucao);
    return;
  }
  // tudo OK, monta a instrução
  monta_instrucao(linha, opcode, arg);
}

// retorna true se o caractere for um espaço (ou tab)
bool espaco(char c)
{
  return c == ' ' || c == '\t';
}

// encontra o primeiro caractere espaço (ou tab) na string
char *pula_ate_espaco(char *s)
{
  while (!espaco(*s) && *s != '\0') {
    s++;
  }
  return s;
}

// troca espaços por fim de string
char *detona_espacos(char *s)
{
  while (espaco(*s)) {
    *s = '\0';
    s++;
  }
  return s;
}

char *pula_aspas(char *s)
{
  char aspa = *s;
  s++;
  while (*s != '\0') {
    if (*s == aspa) {
      *s = '\0';
      s++;
      break;
    }
    s++;
  }
  return s;
}

// faz a string terminar no início de um comentário, se houver
// aproveita e termina se chegar no fim de linha
void tira_comentario(char *s)
{
  while(*s != '\0' && *s != ';' && *s != '\n' && *s != '\r') {
    s++;
  }
  *s = '\0';
}

// uma linha montável é formada por [label][ instrucao[ argumento]]
// o que está entre [] é opcional
// as partes são separadas por espaço(s)
// de ';' em diante, ignora-se (comentário)
// a string é alterada, colocando-se NULs no lugar dos espaços, para separá-la em substrings
// quem precisar guardar essas substrings, deve copiá-las.
void monta_string(int linha, char *str)
{
  char *label = NULL;
  char *instrucao = NULL;
  char *arg = NULL;
  tira_comentario(str);
  if (*str == '\0') return;
  if (!espaco(*str)) {
    label = str;
    str = pula_ate_espaco(str);
  }
  str = detona_espacos(str);
  if (*str != '\0') {
    instrucao = str;
    str = pula_ate_espaco(str);
  }
  str = detona_espacos(str);
  if (*str != '\0') {
    arg = str;
    if (*str == '\'' || *str == '"') {
      str = pula_aspas(str);
    } else {
      str = pula_ate_espaco(str);
    }
  }
  str = detona_espacos(str);
  if (*str != '\0') {
    fprintf(stderr, "linha %d: ignorando '%s'\n", linha, str);
  }
  if (label != NULL || instrucao != NULL) {
    monta_linha(linha, label, instrucao, arg);
  }
}

void monta_arquivo(char *nome)
{
  FILE *arq;
  arq = fopen(nome, "r");
  if (arq == NULL) {
    fprintf(stderr, "Não foi possível abrir o arquivo '%s'\n", nome);
    return;
  }
  int nlinha = 1;
  char *linha = NULL;
  size_t nbytes;
  while (getline(&linha, &nbytes, arq) != -1) {
    monta_string(nlinha, linha);
    nlinha++;
  }
  free(linha);
  fclose(arq);
  ref_resolve();
}

void verifica_args(int argc, char *argv[argc])
{
  for (int argi = 1; argi < argc; argi++) {
    if (strcmp(argv[argi], "-e") == 0) {
      argi++;
      if (argi >= argc) {
        fprintf(stderr, "ERRO: falta endereço após '-e'\n");
        exit(1);
      }
      char *fim = argv[argi];
      mem_pos = strtol(fim, &fim, 0);
      if (*fim != '\0') {
        fprintf(stderr, "ERRO: endereço inválido: '%s'\n", argv[argi]);
        exit(1);
      }
    } else {
      nome_fonte = argv[argi];
    }
  }
  if (nome_fonte == NULL) {
    fprintf(stderr, "ERRO: chame como '%s [-e end.inicial] nome_do_arquivo'\n",
            argv[0]);
    exit(1);
  }
}

int main(int argc, char *argv[argc])
{
  verifica_args(argc, argv);
  monta_arquivo(nome_fonte);
  mem_imprime();
  return 0;
}

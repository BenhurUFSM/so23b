#include "instrucao.h"

#include <stddef.h>
#include <string.h>

struct {
  char *nome;
  int num_args;
  opcode_t opcode;
} instrucoes[] = {
  { "NOP",    0,  NOP    },
  { "PARA",   0,  PARA   },
  { "CARGI",  1,  CARGI  },
  { "CARGM",  1,  CARGM  },
  { "CARGX",  1,  CARGX  },
  { "ARMM",   1,  ARMM   },
  { "ARMX",   1,  ARMX   },
  { "TRAX",   0,  TRAX   },
  { "CPXA",   0,  CPXA   },
  { "INCX",   0,  INCX   },
  { "SOMA",   1,  SOMA   },
  { "SUB",    1,  SUB    },
  { "MULT",   1,  MULT   },
  { "DIV",    1,  DIV    },
  { "RESTO",  1,  RESTO  },
  { "NEG",    0,  NEG    },
  { "DESV",   1,  DESV   },
  { "DESVZ",  1,  DESVZ  },
  { "DESVNZ", 1,  DESVNZ },
  { "DESVN",  1,  DESVN  },
  { "DESVP",  1,  DESVP  },
  { "CHAMA",  1,  CHAMA  },
  { "RET",    1,  RET    },
  { "LE",     1,  LE     },
  { "ESCR",   1,  ESCR   },
  { "RETI",   0,  RETI   },
  { "CHAMAC", 0,  CHAMAC },
  // pseudo-instrucoes
  { "VALOR",  1,  VALOR  },
  { "STRING", 1,  STRING },
  { "ESPACO", 1,  ESPACO },
  { "DEFINE", 1,  DEFINE },
};

opcode_t instrucao_opcode(char *nome)
{
  if (nome == NULL) return -1;
  for (int i=0; i<N_OPCODE; i++) {
    if (strcasecmp(instrucoes[i].nome, nome) == 0) {
      return instrucoes[i].opcode;
    }
  }
  return -1;
}

char *instrucao_nome(int opcode)
{
  for (int i=0; i<N_OPCODE; i++) {
    if (instrucoes[i].opcode == opcode) {
      return instrucoes[i].nome;
    }
  }
  return NULL;
}

int instrucao_num_args(int opcode)
{
  for (int i=0; i<N_OPCODE; i++) {
    if (instrucoes[i].opcode == opcode) {
      return instrucoes[i].num_args;
    }
  }
  return -1;
}


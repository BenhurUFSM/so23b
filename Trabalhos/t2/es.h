#ifndef ES_H
#define ES_H

// controlador de dispositivos de entrada e saida

#include <stdbool.h>
#include "err.h"

typedef struct es_t es_t; // declara o tipo como sendo uma estrutura opaca

// tipos de ponteiros de funções que um dispositivo deve implementar para
//   poder ser controlado pelo controlador de E/S
//   todas recebem como primeiros argumentos um ponteiro que identifica o
//   controlador desse dispositivo (o equivalente à controladora de hw)
//   e um inteiro que identifica o dispositivo entre os possivelmente vários
//   desse controlador. Esses valores são fornecidos quando o dispositivo é
//   registrado.
typedef err_t (*f_le_t)(void *controladora, int id, int *endereco);
typedef err_t (*f_escr_t)(void *controladora, int id, int valor);

// aloca e inicializa um controlador de E/S
// retorna NULL em caso de erro
es_t *es_cria(void);

// libera os recursos ocupados pelo controlador
void es_destroi(es_t *self);

// registra um dispositivo, identificado com o valor 'dispositivo', e que
//   será acessado através das funções apontadas pelos ponteiros 'f_le' e
//   'f_escr'
// se 'f_le' ou 'f_escr' for NULL, considera-se que a operação correspondente
//   é inválida.
// retorna false se não foi possível registrar
bool es_registra_dispositivo(es_t *self, int dispositivo,
                             void *controladora, int id,
                             f_le_t f_le, f_escr_t f_escr);

// lê um inteiro de um dispositivo
// retorna ERR_OK se bem sucedido, ou
//   ERR_DISP_INV se dispositivo desconhecido
//   ERR_OP_INV se operação inválida
err_t es_le(es_t *self, int dispositivo, int *pvalor);

// escreve um inteiro em um dispositivo
// retorna ERR_OK se bem sucedido, ou
//   ERR_DISP_INV se dispositivo desconhecido
//   ERR_OP_INV se operação inválida
err_t es_escreve(es_t *self, int dispositivo, int valor);
#endif // ES_H

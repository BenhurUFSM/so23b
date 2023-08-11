; programa de exemplo para SO
; imprime "Oi, mundo!"
;
; um programa asm tem um comando por linha
; a partir do caractere ';' é comentário (ignorado pelo montador)
; um comando tem 3 partes:   [label][ instrução[ argumento]]
; o que está entre [] é opcional (ou depende da instrução)
; 'instrução' pode ser qualquer instrução do processador, ou uma das
; pseudo-instruções
; essas partes são separadas por um ou mais espaços
; não pode ter espaço antes do label
; o argumento pode ser um número ou um label ou um caractere entre aspas
; o valor de um label é definido quando ele aparece no início da linha,
; geralmente com o valor correspondente ao endereço onde a instrução dessa
; linha será colocada na memória. Com a pseudo-instrução DEFINE, pode-se
; definir um valor qualquer para um label.
; pseudo-instruções:
;     VALOR  x     -> coloca o valor x na próxima posição de memória
;     STRING "abc" -> coloca os caracteres a, b, c e \0 na memória
;     ESPACO x     -> coloca 'x' valores 0 nas próximas posições de memória
; lab DEFINE x     -> define o label 'lab' com o valor x e não com o end. atual

; o dispositivo correspondente a tela do primeiro terminal é 2 (veja controle.c)
tela     DEFINE 2
         ; x = 0
         CARGI 0
         TRAX
mais1    ; a = str[x]
         CARGX str
         ; if a==0 goto fim
         DESVZ fim
         ; print a
         ESCR tela
         ; x++
         INCX
         ; goto mais1
         DESV mais1
         ; stop
fim      PARA
str      string 'Oi, mundo!'

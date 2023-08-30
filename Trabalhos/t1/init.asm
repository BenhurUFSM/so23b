; programa de exemplo para SO
; processo inicial do sistema
; cria outros processos, para testar
;

; chamadas de sistema
SO_ESCR  define 2
SO_CRIA_PROC define 7
SO_MATA_PROC define 8

limpa    define 10

         cargi str1
         chama impstr
         cargi limpa
         chama impch
         cargi str2
         chama impstr
         cargi prog
         chama impstr
         cargi prog
         trax
         cargi SO_CRIA_PROC
         chamas
         cargi limpa
         chama impch
         cargi voltou
         chama impstr
morre
         cargi matar
         chama impstr
         cargi 0
         trax
         cargi SO_MATA_PROC
         chamas
         cargi nao_morri
         chama impstr
         desv morre

str1     string 'init inicializando...'
str2     string 'vou executar '
prog     string 'ex3.maq'
voltou   string 'voltou para o init!! '
matar    string 'vou pedir para morrer '
nao_morri string 'nao morri! '

; imprime a string que inicia em A (destroi X)
impstr   espaco 1
         TRAX
impstr1
         CARGX 0
         DESVZ impstrf
         CHAMA impch
         INCX
         DESV impstr1
impstrf  RET impstr

; função que chama o SO para imprimir o caractere em A
; retorna em A o código de erro do SO
; não altera o valor de X
impch    espaco 1
         trax
         armm impch_X
         cargi SO_ESCR
         chamas
         trax
         cargm impch_X
         trax
         ret impch
impch_X  espaco 1 ; para salvar o valor de X


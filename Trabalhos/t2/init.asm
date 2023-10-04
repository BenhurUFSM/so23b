; programa de exemplo para SO
; processo inicial do sistema
; cria outros processos, para testar
;

; chamadas de sistema (ver so.h)
SO_LE          define 1
SO_ESCR        define 2
SO_CRIA_PROC   define 7
SO_MATA_PROC   define 8
SO_ESPERA_PROC define 9

limpa    define 10

         cargi msg_ini
         chama impstr
         cargi limpa
         chama impch
         ; cria os processos
         cargi prog1
         trax
         cargi SO_CRIA_PROC
         chamas
         armm pid1
         cargi prog2
         trax
         cargi SO_CRIA_PROC
         chamas
         armm pid2
         cargi prog3
         trax
         cargi SO_CRIA_PROC
         chamas
         armm pid3
         ; espera os processos terminarem
         cargm pid1
         trax
         cargi SO_ESPERA_PROC
         chamas
         cargm pid2
         trax
         cargi SO_ESPERA_PROC
         chamas
         cargm pid3
         trax
         cargi SO_ESPERA_PROC
         chamas
morre
         cargi msg_fim
         chama impstr
         cargi 0
         trax
         cargi SO_MATA_PROC
         chamas
         cargi nao_morri
         chama impstr
         desv morre

msg_ini  string 'init inicializando...'
prog1    string 'p1.maq'
prog2    string 'p2.maq'
prog3    string 'p3.maq'
pid1     espaco 1
pid2     espaco 1
pid3     espaco 1
msg_fim  string 'init terminando...'
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


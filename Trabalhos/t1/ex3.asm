; programa de exemplo para SO
; imprime testando se o dispositivo está livre
;

tela     DEFINE 2
telaOK   DEFINE 3

         CARGI str1
         CHAMA impstr
         CARGI str2
         CHAMA impstr
         PARA
str1     string 'Oi, mundo! Que lindo dia para escrever um texto longo na '
str2     string 'tela do terminal, sem ser interrompido por erros.'

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
SO_ESCR  define 2  ; ver so.h
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


; programa de exemplo para SO
; imprime "Oi, mundo!"
; versao com chamada de sistema

SO_ESCR  define 2  ; ver so.h
         ; x = 0
         CARGI 0
         TRAX
mais1    ; a = str[x]
         CARGX str
         ; if a==0 goto fim
         DESVZ fim
         ; print a
         chama printa
         ; x++
         INCX
         ; goto mais1
         DESV mais1
         ; stop
fim      PARA

; função que chama o SO para imprimir o caractere em A
; retorna em A o código de erro do SO
; não altera o valor de X
printa   espaco 1
         trax
         armm pra_X
         cargi SO_ESCR
         chamas
         trax
         cargm pra_X
         trax
         ret printa
pra_X    espaco 1 ; para salvar o valor de X

str      string 'Oi, mundo!'

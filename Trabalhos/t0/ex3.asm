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

; imprime o caractere em A (não altera X)
impch    espaco 1
         ; salva o caractere
         ARMM impcht
         ; espera o terminal ficar livre
impch1   LE telaOK
         DESVZ impch1
         ; imprime o caractere salvo
         CARGM impcht
         ESCR tela
         RET impch
impcht   espaco 1
         

; programa de exemplo para SO
; imprime "Oi, mundo!"
; agora com função
;
; esse processador não tem pilha de execução
; a instrução de chamada coloca o endereço de retorno no endereço chamado, e
;   inicia a execução no endereço seguinte - o programa tem que prever espaço
;   para isso
; a instrução de retorno tem que conter o endereço inicial da função que está
;   retornando, para encontrar o endereço de para onde retornar

; Por que este programa (e o ex1) não funciona para uma string grande?
; Consegue alterar para que funcione nesse caso, antes de olhar o ex3?

tela     DEFINE 2

         CARGI str1
         CHAMA impstr
         CARGI str2
         CHAMA impstr
         PARA
str1     string 'Oi, '
str2     string "mundo!"

impstr   espaco 1
         ; x = a
         TRAX
mais1    ; a = m[x]
         CARGX 0
         ; if a==0 goto fim
         DESVZ fim
         ; print a
         ESCR tela
         ; x++
         INCX
         ; goto mais1
         DESV mais1
         ; stop
fim      RET impstr

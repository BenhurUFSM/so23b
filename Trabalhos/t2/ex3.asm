; programa de exemplo para SO
; imprime testando se o dispositivo está livre
;

; chamadas de sistema (ver so.h)
SO_LE          define 1
SO_ESCR        define 2
SO_CRIA_PROC   define 7
SO_MATA_PROC   define 8
SO_ESPERA_PROC define 9

         CARGI str1
         CHAMA impstr
         CARGI str2
         CHAMA impstr
         ; pede ao SO para morrer
         cargi 0
         trax
         cargi SO_MATA_PROC
         chamas
         PARA    ; não deve chegar aqui!
str1     string 'Aqui é o ex3, com um texto longo para demorar para escrever '
str2     string 'na tela do terminal.'

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


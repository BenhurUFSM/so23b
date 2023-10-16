; p1.asm
; programa de exemplo para SO
; faz chamadas de sistema para E/S

; usa pouca CPU e bastante E/S
N        define 50  ; até quanto vai contar
CADA     define 1   ; a cada tantos, imprime o valor atual

         desv main
prog     string 'p3  (pouca CPU, bastante E/S)                                      '

; chamadas de sistema (ver so.h)
SO_LE          define 1
SO_ESCR        define 2
SO_CRIA_PROC   define 7
SO_MATA_PROC   define 8
SO_ESPERA_PROC define 9

main
         chama impr_inicio
         chama principal
         chama impr_fim
         chama morre
         para

morre    espaco 1
         cargi 0
         trax
         cargi SO_MATA_PROC
         chamas
         ret morre

impr_inicio espaco 1
         cargi prog
         chama impstr
         cargi N
         chama impnum
         cargi '/'
         chama impch
         cargi CADA
         chama impnum
         cargi '['
         chama impch
         ret impr_inicio

impr_fim espaco 1
         cargi ']'
         chama impch
         ret impr_fim

principal espaco 1
         cargi 0
         trax
laco     incx
         cpxa
         resto cada
         desvnz pulaimp
         cpxa
         chama impnum
pulaimp  cpxa
         sub ene
         desvnz laco
         ret principal
cada     valor CADA
ene      valor N

; imprime a string que inicia em A (destroi X)
impstr   espaco 1
         trax
impstr1
         cargx 0
         desvz impstrf
         chama impch
         incx
         desv impstr1
impstrf  ret impstr

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

; escreve o valor de A no terminal, em decimal
impnum  espaco 1
        ; ei_num = A
        armm ei_num
        ; if ei_num > 0 goto ei_pos
        desvp ei_pos
        ; if ei_num < 0 goto ei_neg
        desvn ei_neg
        ; print '0'; goto ei_f
        cargi '0'
        chama impch
        desv ei_f
ei_neg
        ; ei_num = -ei_num
        neg
        armm ei_num
        ; print '-'
        cargi '-'
        chama impch
ei_pos
        ; faz ei_mul ser a maior potência de 10 <= ei_num
        ; ei_mul = 1
        cargi 1
        armm ei_mul
ei_1
        ; if ei_mul == ei_num goto ei_3
        cargm ei_mul
        sub ei_num
        desvz ei_3
        ; if ei_mul > ei_num goto ei_2
        desvp ei_2
        ; ei_mul *= 10
        cargm ei_mul
        mult dez
        armm ei_mul
        ; goto ei_1
        desv ei_1
ei_2
        ; ei_mul /= 10
        cargm ei_mul
        div dez
        armm ei_mul
ei_3
        ; print (ei_num/ei_mul) % 10 + '0'
        cargm ei_num
        div ei_mul
        resto dez
        soma a_zero
        chama impch
        ; ei_mul /= 10
        cargm ei_mul
        div dez
        armm ei_mul
        ; if ei_mul > 0 goto ei_3
        desvp ei_3
ei_f
        ; print ' '
        cargi ' '
        chama impch
        ; return
        ret impnum
ei_num  espaco 1
ei_mul  espaco 1
a_zero  valor '0'
dez     valor 10


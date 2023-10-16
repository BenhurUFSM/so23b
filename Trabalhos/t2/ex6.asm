; programa de exemplo para SO
; lê dois números do teclado,
; imprime números entre eles na tela

; definições para o terminal B
TECL    DEFINE 4
TECLOK  DEFINE 5
TELA    DEFINE 6
TELAOK  DEFINE 7

LIMPA   DEFINE 10 ; \n, limpa a linha

        desv main

; função para ler um char da entrada (retorna em A)
lechar  espaco 1
        ; espera enquanto não estiver disponível
lc_1
        le TECLOK    ; espera teclado ficar disponível para leitura
        desvz lc_1
        le TECL
        ret lechar

; função pra pular espaços da entrada, retorna em A o primeiro caractere não espaço
; destroi X
pula_espacos
        espaco 1
pe_1    chama lechar
        trax
        cpxa
        sub pe_esp1
        desvz pe_1
        cpxa
        sub pe_esp2
        desvz pe_1
        cpxa
        ret pula_espacos
; 'espacos' a pular
pe_esp1 valor ' '
pe_esp2 valor 10
        
; função para ler um inteiro da entrada (retorna em A)
; destroi X
leint   espaco 1
        ; inicializa número (0) e sinal (1)
        cargi 0
        armm li_num
        cargi 1
        armm li_sig
        chama pula_espacos
        desv li_2
li_1
        chama lechar
        trax
        cpxa
li_2
        sub a_menos
        desvnz li_3
        ; '-': inverte o sinal
        cargm li_sig
        neg
        armm li_sig
        desv li_1
li_3
        ; if ch < '0' goto li_f
        ; li_dig = ch - '0'
        cpxa
        sub a_zero
        desvn li_f
        armm li_dig
        ; if li_dig > 9 goto li_f
        sub nove
        desvp li_f
        ; li_num = li_num * 10 + li_dig
        cargm li_num
        mult dez
        soma li_dig
        armm li_num
        desv li_1     
li_f
        ; final, o número está em li_num, o sinal em li_sig
        ; foi lido um caractere a mais, infelizmente não tem como devolvê-lo
        ; para a entrada como o scanf faz, vai ser perdido...
        cargm li_num
        mult li_sig
        ret leint
li_num  espaco 1
li_sig  espaco 1
li_dig  espaco 1

; função para escrever um char (em A) na tela
escch   espaco 1
        armm ec_tmp
ec_1
        le TELAOK
        desvz ec_1
        cargm ec_tmp
        escr TELA
        ret escch
ec_tmp  espaco 1

; escreve o valor de A no terminal, em decimal
escint  espaco 1
        ; ei_num = A
        armm ei_num
        ; if ei_num > 0 goto ei_pos
        desvp ei_pos
        ; if ei_num < 0 goto ei_neg
        desvn ei_neg
        ; print '0'; goto ei_f
        cargm a_zero
        chama escch
        desv ei_f
ei_neg
        ; escnum = -escnum
        neg
        armm ei_num
        ; print '-'
        cargm a_menos
        chama escch
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
        chama escch
        ; ei_mul /= 10
        cargm ei_mul
        div dez
        armm ei_mul
        ; if ei_mul > 0 goto ei_3
        desvp ei_3
ei_f
        ; print ' '
        cargi ' '
        chama escch
        ; return
        ret escint
ei_num  espaco 1
ei_mul  espaco 1

; escreve a string apontada por A
escstr  espaco 1
        ; es_x=X; X=A
        trax
        armm es_x
es_1
        ; if mem[X] == \0, goto es_f
        cargx 0
        desvz es_f
        ; print mem[X]
        chama escch
        ; x++; goto es_1
        incx
        desv es_1
es_f
        ; X=es_x
        cargm es_x
        trax
        ret escstr
es_x    espaco 1

main
        ; print "Digite número inicial"
        cargi msg_ini
        chama escstr
        ; ini = leint()
        chama leint
        armm ini
        ; limpa tela
        cargi LIMPA
        chama escch
        ; print "Digite número final"
        cargi msg_fim
        chama escstr
        ; fim = leint()
        chama leint
        armm fim
        ; limpa tela
        cargi LIMPA
        chama escch
        ; X = ini
        cargm ini
        trax
ali
        ; print x
        cpxa
        chama escint
        ; if x++ < fim goto ali
        cpxa
        incx
        sub fim
        desvn ali
        ; fim
        para

; variáveis
ini     espaco 1
fim     espaco 1
; constantes
dez     valor 10
nove    valor 9
a_zero  valor '0'
a_menos valor '-'
msg_ini string "Digite número inicial"
msg_fim string "Digite número final"


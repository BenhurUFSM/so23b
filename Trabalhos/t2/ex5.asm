; programa de exemplo para SO
; pede para o usuário adivinhar um número

; definições para o terminal B
TECL    DEFINE 4
TECLOK  DEFINE 5
TELA    DEFINE 6
TELAOK  DEFINE 7

LIMPA   DEFINE 10 ; \n, limpa a linha

         cargi str1
         chama escstr
laco     chama lechute
         chama vechute
         desvz laco
         para
str1     string 'Olá. Escolhi um número entre 1 e 100. Adivinha qual.       '
str2     valor LIMPA
         string 'Digite um número entre 1 e 100 '

; lê um número do usuário, até que seja válido
lechute  espaco 1
lechute1 cargi str2
         chama escstr
lechute2
         chama leint
         armm lech_tmp
         desvn lechute2
         desvz lechute2
         cargm lech_tmp
         sub centoeum
         desvp lechute2
         cargm lech_tmp
         ret lechute
centoeum valor 101
lech_tmp espaco 1

; ve se chute em A é bom, escreve mensagem, retorna 1 se for
vechute  espaco 1
         armm chute
         cargi LIMPA
         chama escch
         cargm chute
         chama escint
         cargm chute
         sub segredo
         desvz chuteok
         desvp chuteg
         cargi msg_peq
         desv vechute1
chuteg   cargi msg_gr
vechute1 chama escstr
         cargi 0
         ret vechute
chuteok  cargi msg_ok
         chama escstr
         cargi 1
         ret vechute
msg_peq  string 'muito pequeno, tente novamente '
msg_gr   string 'muito grande, tente novamente '
msg_ok   string 'parabéns, você acertou!!'
chute    espaco 1
segredo  valor 42

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

; constantes
dez     valor 10
nove    valor 9
a_zero  valor '0'
a_menos valor '-'

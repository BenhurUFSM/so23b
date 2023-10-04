; programa de exemplo para SO
; pede para o usuário adivinhar a letra secreta

teclado  define 0
teclOK   define 1
tela     define 2
telaOK   define 3

         cargi str1
         chama impstr
laco     chama lechute
         chama vechute
         desvz laco
         para
str1     string 'Olá. Escolhi uma letra minúscula. Adivinha qual.       '
str2     valor 10 ; \n, limpa a linha
         string 'Digite uma letra minúscula '

; le um caractere do usuario, até que seja minúscula
lechute  espaco 1
lechute1 cargi str2
         chama impstr
lechute2
         chama lechar
         armm lechtmp
         sub ch_a
         desvn lechute2
         cargm lechtmp
         sub ch_z
         desvp lechute2
         cargm lechtmp
         ret lechute
ch_a     valor 'a'
ch_z     valor 'z'
lechtmp  espaco 1

; le um caractere do teclado
lechar   espaco 1
lechar1  le teclOK
         desvz lechar1
         le teclado
         ret lechar

; ve se chute em A é bom, escreve mensagem, retorna 1 se for
vechute  espaco 1
         armm chute
         cargi msg_chut
         chama impstr
         cargm chute
         sub segredo
         desvz chuteok
         desvp chuteg
         cargi msg_peq
         desv vechute1
chuteg   cargi msg_gr
vechute1 chama impstr
         cargi 0
         ret vechute
chuteok  cargi msg_ok
         chama impstr
         cargi 1
         ret vechute
msg_peq  string 'muito pequeno, tente novamente '
msg_gr   string 'muito grande, tente novamente '
msg_ok   string 'parabéns, você acertou!!'
msg_chut valor 10
         valor "'"
chute    espaco 1
         string "' "
segredo  valor 'k'

; imprime a string que inicia em A (destroi X)
impstr   espaco 1
         trax
impstr1
         cargx 0
         desvz impstrf
         chama impch
         incx
         desV impstr1
impstrf  ret impstr

; imprime o caractere em A (não altera X)
impch    espaco 1
         ; salva o caractere
         armm impcht
         ; espera o terminal ficar livre
impch1   le telaOK
         desvz impch1
         ; imprime o caractere salvo
         cargm impcht
         escr tela
         ret impch
impcht   espaco 1
         

; rotina para tratar uma interrupção
; simplesmente chama o SO
; deve ser montada no endereço 10, que é para onde a CPU desvia quando
;   recebe uma interrupção
trata_irq ; o registrador A tem a requisição de interrupção (irq)
          ; a instrução chamac vai passar o A como argumento
          chamac      ; chama a rotina em C que executa o SO
          ; o valor retornado em A por chamac está sendo ignorado, vai ser
          ;   destruído pela instrução reti
          reti        ; retorna da interrupção (carrega os registradores
                      ;   a partir do endereço 0, onde o SO deve ter colocado)

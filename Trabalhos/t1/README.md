## t1 - implementação de processos

### Alterações no código em relação ao t0

- a CPU agora tem 2 modos de execução, `usuario` e `supervisor`
   - em modo usuário, as instruções LE, ESCR, PARA causam erro (ERR_INSTR_PRIV)
- a CPU agora pode atender a uma interrupção
   - foram definidos vários motivos para isso ("IRQs", estão no novo arquivo `irq.h`)
   - quando atende uma interrupção, a CPU
      - salva todo seu estado interno na memória, a partir do endereço 0
      - altera o registrador A para o IRQ que causou a interrupção
      - altera o registrador de erro para ERR_OK
      - altera o modo para supervisor
      - altera o PC para 10 (o tratador de interrupção deve ser colocado aí)
- implementação de nova instrução, RETI, retorna de interrupção
- a CPU gera uma interrupção interna caso detecte um erro em modo usuário, em vez de repassar para o controlador
- a CPU inicia gerando uma interrupção IRQ_RESET
- o montador tem a opção '-e endereço', para definir o endereço de carga do programa
- implementação de nova instrução, CHAMAC, que permite ao programa em assembly chamar código compilado; gambiarra para poder implementar o SO em C, simulando que ele está sendo executado pelo processadorzinho (instrução privilegiada). Colocada uma operação na API da CPU para informar a função a ser chamada.

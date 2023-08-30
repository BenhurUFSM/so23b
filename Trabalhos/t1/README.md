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
- implementação de nova instrução, CHAMAC, que permite ao programa em assembly chamar código compilado; gambiarra para poder implementar o SO em C, simulando que ele está sendo executado pelo processador. Essa instrução é privilegiada. Colocada uma operação na API da CPU para informar a função a ser chamada quando essa instrução for executada.
- implementação de nova instrução, CHAMAS, que causa uma interrupção do tipo IRQ_SISTEMA, usada para se implementar chamadas ao sistema
- definição de 4 chamadas de sistema:
   - SO_LE para ler um caractere da entrada
   - SO_ESCR para escrever um caractere na saída
   - SO_CRIA_PROC para criar um processo
   - SO_MATA_PROC para matar um processo
- implementação inicial do SO, com:
   - inicializa memória com código para tratar interrupção, desviando para a função so_trata_interrupção
   - carrega programa init.maq, e inicializa PC para o endereço inicial
   - tratador de interrupção que executa uma função específica dependendo do tipo da interrupção
   - implementação parcial das 4 chamadas de sistema, as de E/S são sempre com o terminal A e com espera ocupada; a de criação de processo carrega o novo programa e desvia para ele, mas não tem implementação de processo

### Parte I

Faça uma implementação inicial de processos:
- crie um tipo de dados que é uma estrutura que contém a informação a respeito de um processo
- crie uma tabela de processos, um vetor dessas estruturas
- inicialize essa estrutura na criação do init
- crie uma variável global que designe o processo em execução. Faça de tal forma que tenha suporte a não ter nenhum processo em execução
- faça funções para salvar o estado do processador na tabela de processos (na entrada correspondente ao processo em execução) e para recuperar o estado do processador a partir da tabela
- faça uma função escalonador que escolhe o próximo processo a executar (pode ser bem simples, se o processo que estava em execução estiver pronto continua sendo executado e se não, escolhe o primeiro que encontrar na tabela que esteja pronto)
- o tratamento de interrupção passa a ser:
   - salva o estado do processador
   - executa o tratador correspondente à interrupção
   - executa uma função para tratar de pendências
   - executa o escalonador
   - recupera o estado do processador
- implemente as chamadas de criação e morte de processos
- altere as chamadas de E/S, para usar um terminal diferente dependendo do pid do processo

### Parte II

Na parte I, um processo não bloqueia, se ele não está morto, ele pode executar.
Nesta parte, vamos implementar o bloqueio de processos e eliminar a espera ocupada na E/S.
- nas chamadas de E/S, se o dispositivo não estiver pronto, o SO deve bloquear o processo
- na função que trata de pendências, o SO deve verificar o estado dos dispositivos que causaram bloqueio e desbloquear processos se for o caso
- implemente uma nova chamada de sistema, SO_ESPERA, que bloqueia o processo chamador até que o processo que ele identifica na chamada tenha terminado

### Parte III

Escalonador preemptivo, vamos precisar de interrupções do relógio para isso.

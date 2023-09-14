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

Alterações após ser apresentado em aula
- 13set
   - relogio.[ch]
      - inclusão de um timer configurável, que pode ser usado para gerar interrupção
      - tem 2 novos dispositivos: 
         - "2", que pode ser escrito com o valor do timer, que é decrementado a cada tictac
         - "3", que lê o valor 1 se o timer chegar a 0
      - quando o timer chega em 0, não é automaticamente reinicializado, deve ser colocado um novo valor a cada interrupção para gerar interrupções periódicas
      - o indicador de interrupção não é zerado automaticamente, deve ser zerado pelo tratador de interrupção.
   - cpu.[ch]
      - só aceita interrupções em modo usuário (CPUs reais têm instruções para habilitar e desabilitar interrupções, e é comum desabilitar automaticamente quando aceita uma interrupção)
      - alteração da API, para cpu_interrompe retornar um bool para dizer se a interrupção foi aceita
   - controle.c
      - verifica o relógio a cada instrução e gera uma interrupção se o timer expirar
   - so.c
      - programa o timer do relógio para interromper a cada 50 instruções
      - implementa um tratador para a interrupção do relógio, que só reinicializa o timer
   - so.h README
      - renomeia SO_ESPERA para SO_ESPERA_PROC
      - coloca a definição de SO_ESPERA_PROC no .h
   - Makefile init.asm p[123].asm main.c
      - aumento da memória do sistema para +-10k
      - novos programas, com diferentes relações entre uso de CPU e E/S
      - criação de 3 processos no init, para executar os 3 novos programas
- 14set
   - descrição da parte III


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
- implemente uma nova chamada de sistema, SO_ESPERA_PROC, que bloqueia o processo chamador até que o processo que ele identifica na chamada tenha terminado

### Parte III

Implemente um escalonador preemptivo *round-robin* (circular):
- os processos prontos são colocados em uma fila
- o escalonador sempre escolhe o primeiro da fila
- quando um processo fica pronto (é criado ou desbloqueia), vai para o final da fila
- se terminar o *quantum* de um processo, ele é colocado no final da fila (preempção)

O *quantum* é definido como um múltiplo do intervalo de interrupção do relógio (em outras palavras, o *quantum* equivale a tantas interrupções). Quando um processo é selecionado para executar, tem o direito de executar durante o tempo de um quantum. Uma forma simples de implementar isso é ter uma variável do SO, controlada pelo escalonador, que é inicializada com o valor do quantum (em interrupções) quando um processo diferente do que foi interrompido é selecionado. Cada vez que recebe uma interrupção do relógio, decrementa essa variável. Quando essa variável chega a zero, o processo corrente é movido para o final da fila.

Implemente um segundo escalonador, semelhante ao circular: os processos têm um quantum, e sofrem preempção quando esse quantum é excedido. Os processos têm também uma prioridade, e o escalonador escolhe o processo com maior prioridade entre os prontos.
A prioridade de um processo é calculada da seguinte forma:
- quando um processo é criado, recebe prioridade 0,5
- quando um processo perde o processador (porque bloqueou ou porque foi preemptado), a prioridade é calculada como `prio = (prio + t_exec/t_quantum)`, onde t_exec é o tempo desde que ele foi escolhido para executar e t_quantum é o tempo do quantum.


O SO deve manter algumas métricas, que devem ser apresentadas no final da execução (quando o init morrer):
- número de processos criados
- tempo total de execução
- tempo total em que o sistema ficou ocioso (todos os processos bloqueados)
- número de interrupções recebidas de cada tipo
- número de preempções
- tempo de retorno de cada processo (diferença entre data do término e da criação)
- número de preempções de cada processo
- número de vezes que cada processo entrou em cada estado (pronto, bloqueado, executando)
- tempo total de cada processo em cada estado (pronto, bloqueado, executando)
- tempo médio de resposta de cada processo (tempo médio em estado pronto)

Gere um relatório de execuções do sistema em diferentes configurações.

Obs.: se quiser mudar a velocidade da simulação, dá para alterar o timeout em console.c.

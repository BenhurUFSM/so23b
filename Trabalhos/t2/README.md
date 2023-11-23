## t2 - implementação de memória virtual

### Alterações no código em relação ao t1

- implementação da MMU em `mmu.[hc]`, e da tabela de páginas em `tabpag.[hc]`
- a CPU agora faz todos os acessos à memória usando a MMU
- novo erro `ERR_PAG_AUSENTE`, para indicar falta de página
- o SO tem acesso à MMU na inicialização
- `cpu_modo_t` movido para arquivo próprio por problemas na ordem de includes

- coloca as instruções a executar em uma interrupção diretamente na memória,
  dispensando a necessidade de `trata_irq.asm`
- monta todos os .asm para serem carregados no endereço 0, agora sem memória virtual não dá para executá-los
- carga de programa no próximo endereço livre, altera tabela de páginas para mapear o endereço virtual 0 para o endereço de carga

### Alterações posteriores à cópia do t1

- 10nov
   - cópia do t1, da correção do bug em que a CPU estava causando interrupção quando a 
     instrução RETI coloca a CPU em ERR_CPU_PARADA
   - cópia do t1, da correção do comentário sobre o retorno da chamada de criação de 
     processo

### Descrição

No t1, foi implementado o suporte a processos, mas tem 2 problemas sérios:
- o endereço de carga de cada programa deve ser diferente, para que não tenha conflito de endereços entre os programas em execução. Não é possível executar o mesmo programa em 2 processos diferentes sem montá-lo para ser carregado em um endereço diferente.
- não há proteção de memória entre os processo. Não tem sequer uma forma de detectar se um processo fez um acesso ilegal.

O t2 visa sanar esses problemas, implementando memória virtual. Além disso, vai ser possível executar programas mesmo que a memória necessária para sua execução seja superior à disponível no sistema. A memória virtual será implementada usando paginação.

Para essa implementação, vamos necessitar:
- uma MMU, para fazer a tradução entre endereços virtuais e físicos, usando uma tabela de páginas;
- uma memória secundária, onde o conteúdo das páginas dos processos são mantidos enquanto elas não estiverem na memória principal;
- manter uma tabela de páginas por processo, e atualizá-la a cada troca de processo;
- alterar a carga de programas em memória, usando a memória secundária;
- atender a interrupções de falta de página, e agir de acordo, em caso de acesso ilegal ou troca de páginas entre a memória principal e a secundária;
- implementar um algoritmo de substituição de páginas (ou dois)
- medir o desempenho da paginação

#### MMU e tabela de páginas

A implementação da MMU e de tabela de páginas está sendo fornecida.

#### Memória secundária

A memória secundária tipicamente é implementada usando dispositivos de armazenamento (discos). Durante uma troca, o processo fica bloqueado enquanto as transferências são realizadar no disco.
No nosso caso, vamos simplificar e usar a mesma implementação da memória primária (`mem_t`), e bloquear o processo por tempo.

Vamos considerar que o tempo de transferência de uma página da memória principal para a secundária é sempre o mesmo (e é uma configuração do sistema).
A memória secundária mantém uma variável que diz quando o disco estará livre (ou se já está). Quando uma troca de página é necessária, se o disco estiver livre, atualiza-se esse tempo para "agora" mais o tempo de espera; se não estiver, soma-se o tempo de espera a essa variável. De qualquer forma, o valor da variável indicará a data até a qual o processo deve ser bloqueado por causa dessa troca de página.

#### Uma tabela de páginas por processo

Quando um processo é criado, deve ser também criada uma tabela de páginas para o mapeamento das suas páginas virtuais nos quadros da memória física. A função `tabpag_cria` cria uma tabela vazia.

Quando um processo morre, sua tabela de páginas deve ser destruída (com `tabpag_destroi`), e as páginas de memória secundária e os quadros de memória real que o processo estiver ocupando devem ser liberados.

Uma forma simples de escolher onde colocar o processo novo é carregá-lo completamente em memória secundária quando ele é criado, sem alocar nenhum quadro da memória principal para ele. Dessa forma nenhuma página está mapeada, a tabela de páginas pode ficar vazia. Quando o processo executar, causará faltas de página, e caberá ao algoritmo de substituição de páginas decidir onde colocar as páginas do processo.

Quando o SO despacha um processo para executar, além de atualizar o estado do processador para corresponder ao do processo, deve também configurar a MMU para que utilize a tabela de páginas do processo.
Não é necessário atualizar a tabela de páginas do processo quando inicia o tratamento de uma interrupção, porque na implementação fornecida o MMU usa e altera a própria tabela do processo, e não uma cópia.

#### Carga de programas

Como descrito acima, uma forma simples de gerenciar a memória secundária é alocar memória suficiente para todo o processo em memória secundária durante a carga do processo. Mais simples ainda se essa memória toda for mantida durante toda a execução. Essa simplificação tem um custo: o processo ocupa todo o seu espaço na memória secundária e mais os quadros de memória principal que estiver usando. Em geral, os SOs otimizam isso, liberando páginas da memória secundária que forem copiadas para a memória principal, e realocando quando elas voltarem para a memória secundária.

O SO não suporta alocação dinâmica, então os processos não alteram seus tamanhos durante a execução. O gerenciamento da memória secundária com essas restrições não é complicado, mais ainda se alocar a memória secundária de forma contígua.

#### Atendimento à interrupção de falta de página

O sistema pode gerar dois tipos de interrupção por problemas no acesso à memória, `ERR_END_INV` e `ERR_PAG_AUSENTE`. Existem três situações em que essas interrupções são geradas:
- quando a MMU traduzir um endereço virtual para um endereço físico mas o endereço não é reconhecido pela memória (ERR_END_INV);
- quando a página do endereço virtual não existir na tabela de páginas (ERR_END_INV);
- quando a página está na tabela de páginas mas está marcada como inválida (ERR_PAG_AUSENTE).

O primeiro caso não deveria acontecer, e indica que o SO programou mal a MMU, apontando uma página para um quadro que está fora dos limites da memória física.

O terceiro caso é uma falta de página: o processo está tentando acessar uma posição de memória válida, mas ela no momento não está na memória principal.

O segundo caso pode representar duas possibilidades, pela forma como a tabela de páginas está implementada. Pode ser um acesso a um endereço fora do espaço de endereçamento do processo (e o SO deve matar o processo por "*segmentation fault*") ou pode ser uma falta de página. Para identificar qual é o problema, o SO deve verificar se o endereço virtual que causou o problema pertence ou não ao processo.

O endereço virtual que causou a interrupção é colocado no registrador `complemento`, que deve ter sido salvo na tabela de processos.

No caso de falta de página, o SO deve:
- encontrar um quadro livre na memória principal
- se não houver, deve liberar um quadro ocupado
  - escolher uma página que esteja em memória principal (esse é o algoritmo de substituição de página)
  - copiar a página para a memória secundária se for necessário (se ela foi alterada desde que foi copiada para a memória principal)
  - marcar essa página como não mapeada na tabela de páginas que a contém (pode ser de um processo diferente do que causou a falha de página)
  - o quadro onde a página estava agora está livre
- ler a página necessária da memória secundária para o quadro livre
- marcar o quadro como não livre
- alterar a tabela de páginas do processo, para indicar que essa página está nesse quadro
- bloquear o processo por um tempo correspondente a um ou dois acessos ao disco

### Algoritmo de substituição de página

Implemente os algoritmos FIFO e segunda chance. Cuidado com a escolha da frequência com que o bit de acesso dos quadros de memória é zerado para o segunda chance, se for muito baixa a frequência, grande parte dos quadros estará marcada como acessada, e se for muito alta, bem poucos estarão, afetando o desempenho do algoritmo.

### Medição do sistema de memória virtual

Faça seu programa contar o número de falhas de página atendidas para cada processo.
Defina a memória principal com tamanho suficiente para conter todas as páginas de todos os processos. Executando dessa forma com paginação por demanda, cada processo deve gerar um número de falhas de página igual ao número de páginas que corresponde ao seu tamanho.

Altere o tamanho da memória para ter metade das páginas necessárias para conter todos os processos. Execute nessa configuração e compare o número de falhas de página e os tempos de execução dos programas em relação à configuração anterior.

Continue diminuindo o tamanho da memória pela metade e refazendo a comparação.

Qual o tamanho mínimo de memória que permite a execução dos processos?

Faça o experimento com 2 tamanhos de página, um bem pequeno (algumas palavras) e outro pelo menos 4 vezes maior. Analise as diferenças no comportamento do sistema. Faça um relatório com suas observações e análises.


### Respostas a perguntas

Os programas agora estão sendo montados para serem executados no endereço 0.
O PC deve ser inicializado com 0, porque é nesse endereço que o programa vai procurar sua primeira instrução.

Mas o endereço físico 0 não pode ser usado por um programa, porque é onde a CPU coloca os registradores quando aceita uma interrupção. O endereço 10 também não pode ser usado porque é para onde a execução vai ser desviada para tratar uma interrupção.

Então, para que o programa possa ser executado, deve haver mapeamento de endereços, entre os endereços virtuais gerados pelo programa e os endereços físicos correspondentes.
Quem faz essa tradução é a MMU, quando ela é carregada com uma tabela de páginas. 
Sem tabela de páginas, a MMU simplesmente repassa o endereço virtual que ela recebe para a memória principal como se fosse o endereço físico, e isso não tem chance de funcionar.

Digamos que queiramos implementar somente essa tradução de endereços, sem memória secundária.
Suponha que uma página tenha 10 palavras (é o tamanho que está definido em `tabpag.h`), e que o primeiro processo do sistema ocupe 205 palavras (o espaço de endereçamento virtual dele compreende os endereços entre 0 e 204). Digamos que o SO resolva carregar esse processo a partir do endereço 20 (o início do primeiro quadro livre, já que não pode usar o endereço físico 0 nem 10). O programa então vai ser colocado nos endereços entre 20 e 224.
Para que a MMU traduza o endereço 0 do processo no endereço 20 da memória, a tabela de páginas desse processo deve mapear a página 0 do processo (endereços entre 0 e 9) no quadro 2 da memória (endereços entre 20 e 29).
Da mesma forma, a página 1 deve ser mapeada no quadro 3, a página 20 no quadro 22.

Quando esse processo executar, essa tabela deve ser colocada na MMU, ou o acesso do processo à sua memória não funcionará.

Digamos que agora um segundo processo seja criado, e que o SO resolva carregar esse programa no próximo endereço livre da memória principal (endereço 230, início do quadro 23). O endereço virtual 0 desse processo deve ser mapeado para o endereço 230 (a página 0 deve ser mapeada para o quadro 23), então ele precisa de uma tabela de páginas diferente.

Resumindo, cada processo precisa de uma tabela de páginas, que tem que ser carregada na MMU quando esse processo for escolhido para executar.

Com memória secundária, cada página do processo pode estar em algum quadro da memória principal ou em algum lugar da memória secundária. A tabela de páginas só tem informação no primeiro caso, e a MMU só consegue resolver nesse caso. Se a MMU não consegue resolver, será gerada uma interrupção.

A sugestão é implementar paginação por demanda, com alocação contígua de memória secundária: quando um programa é carregado, encontra uma região na memória secundária que tenha quadros contíguos suficientes para conter todas as páginas do programa, e carrega o programa para lá. Cria uma tabela de páginas vazia para o processo. Quando o processo executar, antes de executar a primeira instrução vai causar uma interrupção de falha de página. É fácil encontrar a página necessária na memória secundária. Em uma primeira versão, aloca a memória primária com tamanho suficiente para todas as páginas de todos os processos, então será fácil encontrar um quadro de memória livre para onde copiar a página, sem a necessidade de um algoritmo de substituição.

Com isso funcionando (todos os processos executando corretamente mesmo com suas páginas espalhadas pela memória e sendo carregados por demanda), dá para reduzir a memória principal e testar o algoritmo de substituição.

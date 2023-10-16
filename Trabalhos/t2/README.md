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

Uma forma simples de escolher onde colocar o processo novo é carregá-lo complegamente em memória secundária quando ele é criado. Dessa forma nenhuma página está mapeada, a tabela de páginas pode ficar vazia. Quando o processo executar, causará faltas de página, e caberá ao algoritmo de substituição de páginas decidir onde colocar as páginas do processo.

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

...

### Medição do sistema de memória virtual

...

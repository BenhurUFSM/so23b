### Algoritmo de substituição de páginas WSClock

A descrição do algoritmo WSClock no livro deixa um pouco a desejar.
Vou tentar esclarecer um pouco.

A ideia básica do algoritmo é o conjunto de trabalho, o conjunto de páginas que um processo precisa que estejam na memória principal para que ele não cause muitas falhas de página.
Esse conjunto é dinâmico, varia conforme a execução do processo evolui.

O conjunto de trabalho de um processo é definido como as páginas que foram acessadas pelo processo para a execução das últimas `w` instruções.
Uma aproximação, usada no algoritmo WSClock, é definir o conjunto de trabalho como as páginas acessadas pelo processo nos últimos `𝜏` tempos de execução do processo (geralmente, esse tempo é medido em interrupções de relógio).

O algoritmo necessita saber há quanto tempo cada página foi acessada pela última vez (em relação ao tempo de execução do processo). A forma como ele faz isso é mantendo um contador para cada processo, que conta o tempo de execução do processo. Cada vez que vem uma interrupção de relógio, ele incrementa esse contador para o processo que foi interrompido (o processo em execução).
Ele mantém também para cada página, a informação do processo que é dono da página e a data do último acesso.
Na interrupção do relógio, o SO percorre todas as páginas em memória, e para as que foram acessadas (as que têm o bit `R` em 1), zera esse bit e copia a data do processo para a data de último acesso da página.
Algoritmicamente:
```
   // tproc[]: tempo de cada processo
   // procexec: o processo em execução
   // ref[]: bit de referenciado de cada página
   // tpag[]: tempo da última referência de cada página
   interrupção de relógio:
      tproc[procexec]++
      para cada página pag na tabela de página corrente (a tabela do procexec)
        if ref[pag] == 1
           ref[pag] = 0
           tpag[pag] = tproc[procexec]
```
O algoritmo WSClock é executado quando tem uma falta de página e tem que ser escolhida uma página para desocupar um quadro para a página que deve vir para a memória principal. Algo assim:
```
   wsclock:
     para cada página pag na memória principal // o "clock" do algoritmo
       proc = processo dono de pag
       if ref[pag] == 1  // a página foi referenciada, atualiza o tempo dela
         ref[pag] = 0
         tpag[pag] = tproc[proc]
       else if tproc[proc] - tpag[pag] > 𝜏  // não tá no conjunto de trabalho
         if alt[pag] == 1   // a página foi alterada
           poe na fila de gravação para a memória secundária
         else
           pag é a página escolhida
```
Algumas observações:
- pode ter um limite no número de páginas que são colocadas na fila de gravação, para não sobrecarregar o disco.
- pode ser que não seja encontrada nenhuma página não alterada fora de conjuntos de trabalho. Nesse caso, tem que aguardar uma das que foram para a fila ser gravada para realizar a escolha (em uma próxima execução do SO; enquanto isso o processo fica bloqueado).
- pode ser todas as páginas estejam em algum conjunto de trabalho. Nesse caso, pode ser escolhida a com acesso menos recente. Essa informação pode tambér ser usada para suspender a execução de algum processo, já que é indicativo de risco de paginação excessiva.
- se houver processo suspenso, as páginas que pertencem a ele podem ser consideradas para serem escolhidas independentemente do tempo do último acesso.

## t1 - implementação de processos

### Alterações no código em relação ao t0

- a cpu agora tem 2 modos de execução, `usuario` e `supervisor`; inicia em modo supervisor

   em modo usuário, as instruções LE, ESCR, PARA causam erro (ERR_INSTR_PRIV)


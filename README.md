# SO
Trabalho Prático da cadeira de Sistemas Operativos
Uma simulação de um orchestrator de processos de um sistema operativo.
Ficou quase como desejado, porém tem dois problemas.
  Um dos objetivos de guardar a informação após executar um comando cria um novo processo desnecessariamente.
  Os processos-pai não fazem waitpid em nenhum momento e então após os processos-filho executarem ficam zombie e consomem memória desnecessariamente.

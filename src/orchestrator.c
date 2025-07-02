#include "struct.h"

// Forma de correr o orchestrator
// ->/orchestrator output_folder parallel-tasks sched-policy
// sendo output_folder para onde vão os ficheiros das tarefas feitas executadas,
// parallel-tasks o máximo de tarefas a serem corridas em paralelo,
// a sched policy a política de escalonamento(vamos tentar fazer pelo menos 3)
// políticas de escalonamento: FCFS, SJF e CUSTOM
// sempre que um cliente entrar em contacto, incrementar 1 e retornar
// gettimeofday(&timeval, NULL);

void swapminfo(minfo a, minfo b){
    minfo tmp;
    *tmp = *a;
    *a = *b;
    *b = *tmp;
}
long int time_diff(struct timeval *start, struct timeval *end) {
    return (1e+3 *(end->tv_sec - start->tv_sec)) + (1e-3 * (end->tv_usec - start->tv_usec));
}

char* escritanooutput(minfo mensagem){
    // Formata a mensagem de saída para escrever no ficheiro de output
    char* realoutput = malloc(128 + sizeExecute * sizeof(char));
    sprintf(realoutput, 
    "------------------\n  TASK (id %d, pid %d)\n   TIME %d miliseconds\n   COMMAND %s\n   OUTPUT %s\n------------------\n"
    , mensagem->id, mensagem->pid, mensagem->time, mensagem->nome, mensagem->execucao);
    return realoutput;
}

// Função para implementar a politica de esclonamento SJF
int sc_SJF(minfo (*fila)[], int N){
    int menor_tempo = (*fila)[0]->time;
    int indice_menor_tempo = 0;
    // Procura a tarefa com o menor tempo
    for (int i = 1; i< N; i++){
        if((*fila)[i]->time < menor_tempo){
            menor_tempo = (*fila)[i]->time;
            indice_menor_tempo = i;
        }
    }
    
    // Move a tarefa com menor tempo para o final da fila
    for(int i=indice_menor_tempo;i<N-1;i++){
        swapminfo((*fila)[i],(*fila)[i+1]);
    }
    indice_menor_tempo = N-1;
    return indice_menor_tempo;
}

// Função para implementar uma politica de escalonamento CUSTOM
int sc_CUSTOM(minfo (*fila)[], int N){ // se um gajo for ultrapassado, o mensagem->custom dele soma 1
    int menor_tempo = (*fila)[0]->time; // se o mensagem->custom dele chegar a 3, ele é o escolhido->
    int indice_escolhido = 0;
    
    // Procura a tarefa com o menor tempo
    for (int i = 0; i< N; i++){
        if((*fila)[i]->custom >=3){
            indice_escolhido = i;
            break;
        }else{
            if((*fila)[i]->time < menor_tempo){
                menor_tempo = (*fila)[i]->time;
                indice_escolhido = i;
            }
        }
    }
    
    // Move a tarefa com menor tempo para o final da fila
    for(int i=indice_escolhido;i<N-1;i++){
        swapminfo((*fila)[i],(*fila)[i+1]);
    }
    indice_escolhido = N-1;
    return indice_escolhido;
}

int main(int argc, char * argv[]){
    
    // Verifica os argumentos passados
    int N = atoi(argv[2]);
    char* sp = argv[3];
    if((0 != strcmp(argv[1],"output-folder"))){
        perror("Erro no input");
        return -1;
    }
    if(N<0){
        perror("Erro no input");
        return -1;
    }    
    if(0 != (strcmp(sp, "FCFS"))){
        if(0 != (strcmp(sp, "SJF"))){
            if(0 != (strcmp(sp, "CUSTOM"))){
                perror("Erro no input");
                return -1;
            }
        }
    }

    // Inicializa variáveis
    int tempotestes = 0; // tempo de execução total
    int countID = 1001; // id das mensagens
    int countPT = 0; // quantos processos estão a decorrer neste momento
    int countfila = 0; // quantos processos estão na fila
    minfo filaEspera[10*N];
    minfo mensagem = malloc(sizeof(struct minfo));

    // Cria o FIFO do servidor
    if(mkfifo(SERVER, 0666)==-1){
		perror("Erro ao criar o fifo");
		return -1;
	}

	printf("--- Fifo do server criado ---\n");
    int fifoserver_fd = open(SERVER, O_RDWR, 0666);
	printf("--- Fifo do server aberto ---\n");

    // Filho que vai parar o programa se escrever stop no input
    int pipedostop[2];
    pipe(pipedostop);
    int pidstop = fork();
    if(pidstop==0){
        char stop[5] = "nstp";
        while(strcmp(stop,"stop")!=0){
            read(0, stop, 4);
        }
        minfo mensagemstop = malloc(sizeof(struct minfo));
        mensagemstop->pid = 0;
        write(fifoserver_fd, mensagemstop, sizeof(struct minfo));
        _exit(0);
    }
    while(1){
        printf("Começa o read\n");
	    int read_bytes = read(fifoserver_fd, mensagem, sizeof(struct minfo)); // Não precisas de verificar se a mensagem
                                                                              //está correta porque isso é visto no cliente
        if(mensagem->pid == 0){
            printf("Tempo total: %d milisegundos\n", tempotestes);
            unlink(SERVER);
            return 0;
        }
        
        // Processa a mensagem recebida
        printf("Acaba o read\n");
        printf("Quantos bytes li: %d\n", read_bytes);
        if(mensagem->tipo == 1){ // Se for uma tarefa a ser escrita no output
            countPT--;
            tempotestes = tempotestes + mensagem->time;
            int pid = fork();
            if(pid==0){
                int fdoutput = open(OUTPUT, O_CREAT|O_APPEND|O_WRONLY, 0777);
                char *realoutput;
                realoutput = escritanooutput(mensagem);
                write(fdoutput,realoutput,strlen(realoutput));
                close(fdoutput);
                _exit(0);
            }
            if((countfila>0) && (countPT<N)){ // Manda fazer um novo processo se tiver processos para fazer
                int pf=0;
                if(0 == strcmp(sp, "SJF")){
                    pf = sc_SJF(&filaEspera, countfila);
                }
                if(0 == strcmp(sp, "CUSTOM")){
                    pf = sc_CUSTOM(&filaEspera, countfila);
                }
                mensagem = filaEspera[pf];
                countfila--;
                countPT++;
                if(mensagem->operacao == 1){ // -u é 1 comando só
                    int pid = fork();
                    if(pid==0){
                        int pipes[2];
                        pipe(pipes);
                        int pidfilho = fork();
                        if(pidfilho==0){
                            close(pipes[0]);
                            dup2(pipes[1],1);
                            close(pipes[1]);
                            char *comando;
                            comando = strdup(mensagem->nome);
                            char *nome = strsep(&comando, " ");
                            execlp(nome, nome, comando, NULL);
                            _exit(0);
                        }
                        close(pipes[1]);
                        read(pipes[0], mensagem->execucao, sizeExecute);
                        mensagem->tipo = 1;
                        gettimeofday(&(mensagem->end), NULL);
                        mensagem->time = time_diff(&mensagem->start, &mensagem->end);

                        int fifoserver_fd = open(SERVER, O_WRONLY, 0666);
                        write(fifoserver_fd,mensagem, sizeof(struct minfo));
                        close(fifoserver_fd);
                        _exit(0);
                    }
                }
                if(mensagem->operacao == 0){ // -p vários comandos
                    int pidpai = fork();
                    if(pidpai==0){
                        char* exec_comandos[10];
                        char *string, *cmd;
                        int rN=0;
                        cmd = strdup(mensagem->nome);
                        while((string = strsep(&cmd,"|"))!=NULL){
                            exec_comandos[rN]=string;
                            rN++;
                        }
                        exec_comandos[rN]=NULL;
                        int pipes[rN][2];
                        for(int i=0;i<rN;i++){
                            if(i==0){
                                pipe(pipes[i]);
                                int pid0 = fork();
                                if(pid0==0){
                                    close(pipes[i][0]);
                                    dup2(pipes[i][1],1);
                                    close(pipes[i][1]);
                                    char *comando;
                                    comando = strdup(exec_comandos[i]);
                                    char *nome = strsep(&comando, " ");
                                    execlp(nome, nome, comando, NULL);
                                    _exit(0);
                                }
                                close(pipes[i][1]);
                            }
                            else if(i==rN-1){
                                int pidfim = fork();
                                if(pidfim == 0){
                                    dup2(pipes[i-1][0],0);
                                    close(pipes[i-1][0]);
                                    dup2(pipes[i][1],1);
                                    close(pipes[i][1]);
                                    char *comando;
                                    comando = strdup(exec_comandos[i]);
                                    char *nome = strsep(&comando, " ");
                                    execlp(nome, nome, comando, NULL);
                                    _exit(0);
                                }
                                close(pipes[i-1][0]);
                            }else{
                                pipe(pipes[i]);
                                int pidmeio = fork();
                                if(pidmeio == 0){
                                    close(pipes[i][0]);
                                    dup2(pipes[i-1][0],0);
                                    close(pipes[i-1][0]);
                                    dup2(pipes[i][1],1);
                                    close(pipes[i][1]);
                                    char *comando;
                                    comando = strdup(exec_comandos[i]);
                                    char *nome = strsep(&comando, " ");
                                    execlp(nome, nome, comando, NULL);
                                    _exit(0);
                                }
                                close(pipes[i-1][0]);
                                close(pipes[i][1]);
                            }
                        }
                        close(pipes[rN][1]);
                        read(pipes[rN][0], mensagem->execucao, sizeExecute);
                        mensagem->tipo = 1;
                        gettimeofday(&(mensagem->end), NULL);
                        mensagem->time = time_diff(&mensagem->start, &mensagem->end);


                        int fifoserver_fd = open(SERVER, O_WRONLY, 0666);
                        write(fifoserver_fd,mensagem, sizeof(struct minfo));
                        close(fifoserver_fd);
                        _exit(0);

                    }
                }
            }
        
        }else if(mensagem->tipo!=1){ // Se for uma tarefa a ser escalonada
            if(mensagem->tipo == 0){
                printf("--- Mensagem lida ---\n");
                if(mensagem->operacao == 2){ // status -- cat do output
                    int pid = fork();
                    
                    if(pid == 0){     // O filho do filho vai executar e o filho vai mandar o output diretamente para o cliente
                        int pipes[2]; 
                        pipe(pipes);
                        int pidfilho = fork();
                        if(pidfilho == 0){
                            close(pipes[0]);
                            dup2(pipes[1], 1);
                            close(pipes[1]);
                            int fdoutput = open(OUTPUT, O_CREAT|O_RDONLY, 0666);
                            execlp("cat", "cat", OUTPUT, NULL);
                            close(fdoutput);
                            _exit(0);
                        }
                        close(pipes[1]);
                        char outputstatus[4096];
                        read(pipes[0], &outputstatus, 4096);
                        
                        char fifoc_name[30];
                        sprintf(fifoc_name, CLIENT "%d", mensagem->pid);
                        int fifocliente_fd = open(fifoc_name, O_WRONLY, 0666);
                        write(fifocliente_fd, &outputstatus, strlen(outputstatus));
                        _exit(0);
                    }
                }else{
                    if((mensagem->operacao == 1)||(mensagem->operacao == 0)){
                        mensagem->id = countID;
                        countID++;
                        int pidparacliente = fork();
                        
                        // Avisa o cliente que já recebeu a mensagem
                        if(pidparacliente==0){
                            char fifoc_name[30];
                            sprintf(fifoc_name, CLIENT "%d", mensagem->pid);
                            int fifocliente_fd = open(fifoc_name, O_WRONLY, 0666);
                            write(fifocliente_fd, mensagem, sizeof(struct minfo));
                            _exit(0);
                        }
                        gettimeofday(&(mensagem->start), NULL);
                        filaEspera[countfila] = mensagem;
                        countfila++;
                        if((countfila>0) && (countPT<N)){ // Manda fazer um novo processo se tiver processos para fazer
                            int pf=0;
                            if(0 == strcmp(sp, "SJF")){
                                pf = sc_SJF(&filaEspera, countfila);
                            }
                            if(0 == strcmp(sp, "CUSTOM")){
                                pf = sc_CUSTOM(&filaEspera, countfila);
                            }
                            mensagem = filaEspera[pf];
                            countfila--;
                            if(mensagem->operacao == 1){ // -u é um comando só
                                int pid = fork();
                                if(pid==0){
                                    int pipes[2];
                                    pipe(pipes);
                                    int pidfilho = fork();
                                    if(pidfilho==0){
                                        close(pipes[0]);
                                        dup2(pipes[1],1);
                                        close(pipes[1]);
                                        char *comando;
                                        comando = strdup(mensagem->nome);
                                        char *nome = strsep(&comando, " ");
                                        execlp(nome, nome, comando, NULL);
                                        _exit(0);
                                    }
                                    close(pipes[1]);
                                    read(pipes[0], mensagem->execucao, sizeExecute);
                                    mensagem->tipo = 1;
                                    gettimeofday(&(mensagem->end), NULL);
                                    mensagem->time = time_diff(&mensagem->start, &mensagem->end);

                                    int fifoserver_fd = open(SERVER, O_WRONLY, 0666);
                                    write(fifoserver_fd,mensagem, sizeof(struct minfo));
                                    close(fifoserver_fd);
                                    _exit(0);
                                }
                            }
                            if(mensagem->operacao == 0){ // -p vários comandos
                                int pidpai = fork();
                                if(pidpai==0){
                                    char* exec_comandos[10];
                                    char *string, *cmd;
                                    int rN=0;
                                    cmd = strdup(mensagem->nome);
                                    while((string = strsep(&cmd,"|"))!=NULL){
                                        exec_comandos[rN]=string;
                                        rN++;
                                    }
                                    exec_comandos[rN]=NULL;
                                    int pipes[rN][2];
                                    for(int i=0;i<rN;i++){
                                        if(i==0){
                                            pipe(pipes[i]);
                                            int pid0 = fork();
                                            if(pid0==0){
                                                close(pipes[i][0]);
                                                dup2(pipes[i][1],1);
                                                close(pipes[i][1]);
                                                char *comando;
                                                comando = strdup(exec_comandos[i]);
                                                char *nome = strsep(&comando, " ");
                                                execlp(nome, nome, comando, NULL);
                                                _exit(0);
                                            }
                                            close(pipes[i][1]);
                                        }else if(i==rN-1){
                                            int pidfim = fork();
                                            if(pidfim == 0){
                                                dup2(pipes[i-1][0],0);
                                                close(pipes[i-1][0]);
                                                dup2(pipes[i][1],1);
                                                close(pipes[i][1]);
                                                char *comando;
                                                comando = strdup(exec_comandos[i]);
                                                char *nome = strsep(&comando, " ");
                                                execlp(nome, nome, comando, NULL);
                                                _exit(0);
                                            }
                                            close(pipes[i-1][0]);
                                        }else{
                                            pipe(pipes[i]);
                                            int pidmeio = fork();
                                            if(pidmeio == 0){
                                                close(pipes[i][0]);
                                                dup2(pipes[i-1][0],0);
                                                close(pipes[i-1][0]);
                                                dup2(pipes[i][1],1);
                                                close(pipes[i][1]);
                                                char *comando;
                                                comando = strdup(exec_comandos[i]);
                                                char *nome = strsep(&comando, " ");
                                                execlp(nome, nome, comando, NULL);
                                                _exit(0);
                                            }
                                            close(pipes[i-1][0]);
                                            close(pipes[i][1]);
                                        }
                                    }
                                    read(pipes[rN][0],mensagem->execucao,sizeExecute);
                                    mensagem->tipo = 1;
                                    gettimeofday(&(mensagem->end), NULL);
                                    mensagem->time = time_diff(&mensagem->start, &mensagem->end);

                                    int fifoserver_fd = open(SERVER, O_WRONLY, 0666);
                                    write(fifoserver_fd,mensagem, sizeof(struct minfo));
                                    close(fifoserver_fd);
                                    _exit(0);

                                }
                            }
                        }
                    }    
                }
            }
        }
    }
    unlink(SERVER);
    return 0;
}
#include "struct.h"

// Forma de correr o client:
// ./client execute 100 -u "prog-a arg-1 (...) arg-n"
//ou
// ./client execute 3000 -p "prog-a arg-1 (...) arg-n | prog-b arg-1 (...) arg-n | prog-c arg-1 (...) arg-n"
// para estes dois o output será o id da tarefa
//ou
// ./client status 
//o output é Executing...\n Scheduled...\n Completed...\n


int main(int argc, char *argv[]){

    //Verifica se o número de argumentos está correto
    if(argc!=2 && argc!=5){
        perror("Número de argumentos inválido!");
        return -1;
    }
    minfo mensagem = malloc(sizeof(struct minfo));
    
    //Preenche a parte geral da "mensagem"
    mensagem->pid = getpid();
    mensagem->tipo = 0;
    mensagem->custom = 0;
    
    //Verifica o tipo de operação e preenche os campos da mensagem
    if (argc == 2 &&  (strcmp(argv[1],"status") == 0)){
        mensagem->operacao = 2;
        mensagem->time = 0;
    
    //Se não for "status", verifica se os argumentos têm os valores corretos
    }else{ 
        if((0 != strcmp(argv[1], "execute")) &&  
        ((0 != strcmp(argv[3],"-u")) || (0 != strcmp(argv[3], "-p"))) &&
        atoi(argv[2])<=0){  //Verificar o argv[4] no server
            perror("Número de argumentos inválido!");
            return -1;
        }
        
        mensagem->time = atoi(argv[2]);
        if(0 == strcmp(argv[3],"-u")){
            mensagem->operacao = 1;
        }else{
            mensagem->operacao = 0;
        }
        sprintf(mensagem->nome, argv[4]);
    }

    //Cria o FIFO do cliente para depois ler
    char fifoc_name[30];
	sprintf(fifoc_name, CLIENT "%d", mensagem->pid);
	
    if(mkfifo(fifoc_name, 0666)==-1){
		perror("Erro ao criar o fifo");
		return -1;
	}

	int fifoserver_fd = open(SERVER, O_WRONLY, 0666);
    
    //Escreve a mensagem no FIFO do servidor
    write(fifoserver_fd,mensagem, sizeof(struct minfo));
	close(fifoserver_fd);

    //Abre o FIFO do cliente para leitura
    int fifocliente_fd = open(fifoc_name, O_RDONLY, 0666);
    
    //Lê e processa a resposta do servidor
    if(mensagem->operacao != 2){
        read(fifocliente_fd, mensagem, sizeof(struct minfo));
        char output[64];
        sprintf(output, "Task (id %d, pid %d) recieved\n", mensagem->id, mensagem->pid);

        write(1,output, strlen(output));
    
    }else if(mensagem->operacao == 2){
        char statusoutput[4097];
        read(fifocliente_fd, &statusoutput, 4096);
        strcat(statusoutput, "\n");
        write(1,statusoutput, strlen(statusoutput));
    }
    
    unlink(fifoc_name);
    return 0;
}

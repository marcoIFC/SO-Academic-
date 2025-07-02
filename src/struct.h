#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>  //chamadas ao sistema: defs e decls essenciais
#include <fcntl.h>  //O_RDONLY, O_WRONLY, O_CREAT, O_*
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>


#define SERVER "fifo_server" 
#define CLIENT "fifo_client"
#define sizeExecute 301
#define OUTPUT "SO-1/output-folder/output"

typedef struct minfo{
    int id;
    int tipo; // 0 se for mensagem normal 
              // 1 se for um filho do server a mandar para o server
    int operacao;//  0 == -p, 1 == -u, 2 == status
    int time;
    int pid; // para entrar em contacto com o cliente
    int custom; // para a política de escalonamento CUSTOM
    struct timeval start; //tempo de inicio atualizar o valor quando chega ao servidor
    struct timeval end; // quando o filho termina de executar
    char execucao[sizeExecute];
    char nome[sizeExecute];
}*minfo;

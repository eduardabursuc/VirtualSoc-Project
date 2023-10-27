#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>

void sigint ( int signal ){
    if( signal == SIGINT ) {
        unlink("client-server");
        printf("\n");
        exit(0);
    }
}

int main() {

    signal(SIGINT, sigint);

    if ( mkfifo("client-server", 0666) < 0 ) {
        printf("Eroare la crearea unui canal de comunicare fifo.\n");
        exit(1);
    }

    char command[32];
    int fifo;

    while( 1 ) {

        if( access("server-client", F_OK) == -1 ) {
            printf("Server offline.\n");
            unlink("client-server");
            exit(1);
        }

        if ( (fifo = open("client-server", O_WRONLY | O_TRUNC)) < 0 ) {
            printf("Eroare la deschiderea unui canal de comunicare fifo.\n");
            exit(1);
        }

        fgets(command, 32, stdin);

        if( write(fifo, command, strlen(command)) < 0 ) {
            printf("Eroare la trasmiterea comenzii catre server.\n");
            exit(1);
        }
        
        close(fifo);
      
        if( (fifo = open("server-client", O_RDONLY )) < 0 ) {
            printf("Eroare la deschiderea canalului de comunicare fifo.\n");
            exit(1);
        }
        
        char response[1024] = "";
        if( read(fifo, response, sizeof(response)) < 0 ){
            printf("Eroare la citire din fifo.\n");
            exit(1);
        }
        close(fifo);

        if( strcmp(response, "quit") == 0 ) {
            unlink("client-server");
            return 0;
        }
        printf("%s\n", response);
          
        
    }


}
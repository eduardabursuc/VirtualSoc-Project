#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>

char *response;

void sigint ( int signal ){
    if( signal == SIGINT ) {
        unlink("client-server");
        printf("\n");
        free(response);
        exit(0);
    }
}



int main() {

    signal(SIGINT, sigint);

    if ( mkfifo("client-server", 0666) < 0 ) {
        printf("Eroare la crearea unui canal de comunicare fifo.\n");
        exit(1);
    }

    
    char command[32] = "";
    int size, fifo;

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
        size = strlen(command)-1;

        if( write(fifo, &size, sizeof(int)) < 0 ) {
            printf("Eroare la trasmiterea comenzii catre server.\n");
            exit(1);
        }

        if( write(fifo, command, size*sizeof(char)) < 0 ) {
            printf("Eroare la trasmiterea comenzii catre server.\n");
            exit(1);
        }

     
        close(fifo);
      
        if( (fifo = open("server-client", O_RDONLY )) < 0 ) {
            printf("Eroare la deschiderea canalului de comunicare fifo.\n");
            exit(1);
        }

        if( read(fifo, &size, sizeof(int)) < 0 ){
            printf("Eroare la citire din fifo.\n");
            exit(1);
        }

        response = (char *)malloc(size*sizeof(char));


        if( read(fifo, response, size) < 0 ){
            printf("Eroare la citire din fifo.\n");
            exit(1);
        }
        close(fifo);

        if( strcmp(response, "quit") == 0 ) {
            unlink("client-server");
            free(response);
            return 0;
        }
        printf("%s\n", response);
        

        
    }


}
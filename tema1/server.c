#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <string.h>
#include <stdbool.h>
#include <utmp.h>
#include <sys/socket.h>
#include <time.h>
#include <signal.h>


void sigint ( int signal ){
    if( signal == SIGINT ) {
        unlink("server-client");
        printf("\n");
        exit(0);
    }
}

char response[1024];

int check_stat() {
    FILE* stat = fopen("logged.txt", "r");
    int* status;
    fscanf(stat, "%d", status);
    fclose(stat);
    return *status;
}

void new_stat(int s) {
    FILE* stat = fopen("logged.txt", "w");
    if( stat == NULL ) {
        printf("Eroare la deschidere fisier logged.txt");
        exit(1);
    }
    fprintf(stat, "%d", s);
    fclose(stat);
}

bool login_user(char username[]) {

    FILE* users = fopen("users.txt", "r");
    if( users == NULL ) {
        printf("Eroare la deschidere fisier users.txt");
        exit(1);
    }
    char usr[128] = "", ch;
    int i=0;
    size_t len = 0;
    while( !feof(users) ) {
        if( (ch = fgetc(users)) == '\n' ){
            if( strcmp(usr, username) == 0 ) {
            fclose(users);
            return true;
            } else {
                for(int j = 0; j <= i ; j++ )
                    usr[j] = '\0';
                i = 0;
                }
        } else {
            usr[i] = ch;
            i++;
        }
            
    }
    fclose(users);
    return false;
    
}

int main() {

    signal(SIGINT, sigint);

    if ( mkfifo("server-client", 0666) < 0 ) {
        printf("Eroare la crearea unui canal de comunicare fifo.\n");
        exit(1);
    }

    new_stat(0);

    while( 1 ) {

    char command[32] = "", response[1024] = "";
    
    int fifo_cs;

    while( (fifo_cs = open("client-server", O_RDONLY )) < 0 ) {
        sleep(1);
    }

    if ( read(fifo_cs, command, sizeof(command)) < 0 || access("client-server", F_OK) < 0 ) {
        // in moementul in care in clientul a facut un quit fortat (sigint)
        // serverul va ramane blocat aici, iar o data ce vom crea din nou 
        // canalul client-server trebuie sa fie deschis inca o data si setat
        // clientul ca default (logged = false)
        while( (fifo_cs = open("client-server", O_RDONLY )) < 0 ) {
        sleep(1);
        }
        new_stat(0);
        if(read(fifo_cs, command, sizeof(command)) < 0) {
            printf("Eroare la citirea din fifo.\n");
            exit(1);
        }
    }

    close(fifo_cs);

    int fd[2], socket[2];

    if( pipe(fd) < 0 ) {
        printf("Eroare la crearea unui canal anonim.\n");
        exit(1);
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, socket) < 0) 
      { 
        perror("Eroare la creare socketpair."); 
        exit(1); 
      } 
    

    command[strlen(command)-1] = '\0';

    if( strncmp(command, "login : ", 8) == 0 ){

//LOGIN

        switch ( fork() ) {
            case -1: 
                    printf("Eroare fork().\n");
                    exit(1);
            case 0:
                    close(fd[0]);
                    if ( check_stat() ) {
                        strcpy(response, "Sunteti autentificati deja.");
                    } else {
                        if( login_user(command+8) ) {
                        new_stat(1);
                        strcpy(response, "Login cu succes.");
                        } else {
                            strcpy(response, "Eroare login.");
                        }
                    }
                    if( write(fd[1], response, strlen(response)) < 0 ) {
                            printf("Eroare la scrierea in canalul anonim.\n");
                            exit(1);
                    }
                    close(fd[1]);
                    exit(0);

            default: 
                    int fifo_sc;
                    if( (fifo_sc = open( "server-client", O_WRONLY | O_TRUNC)) < 0 ) {
                        printf("Eroare la deschiderea fifo.\n");
                        exit(1);
                    }
                    if( read(fd[0], response, 1024) < 0 ) {
                        printf("Eroare la citirea din canalul anonim.\n");
                        exit(1);
                    }
                    close(fd[0]);

                    if( write(fifo_sc, response, strlen(response)) < 0 ) {
                        printf("Eroare la scrierea in fifo.\n");
                        exit(1);
                    }
                    close(fifo_sc);

        }
    } else if(strcmp(command, "logout") == 0) {

//LOGOUT

        if( check_stat() ){

            new_stat(0);
            strcpy(response, "Logout cu succes.");

        } else {
            strcpy(response, "Nu sunteti autentificati.");
        }

        int fifo_sc;
        if( (fifo_sc = open( "server-client", O_WRONLY | O_TRUNC)) < 0 ) {
            printf("Eroare la deschiderea fifo.\n");
            exit(1);
        }

        if( write(fifo_sc, response, strlen(response)) < 0 ) {
            printf("Eroare la scrierea in fifo.\n");
            exit(1);
        }
        close(fifo_sc);
                                

    } else if( strcmp(command, "get-logged-users") == 0 ){

// GET-LOGGED-USERS

        switch( fork() ) {

            case -1:
                    printf("Eroare fork().\n");
                    exit(1);
                
            case 0: 
                    if( check_stat() ){

                    struct utmp *ut;
                    setutent();

                    while( (ut = getutent()) != NULL ) {
                        if( ut -> ut_type == USER_PROCESS) {
                            time_t t = ut->ut_time;
                            struct tm *tm_info = localtime(&t);
                            sprintf(response + strlen(response), "%s     %s     %02d:%02d\n", ut->ut_user, ut->ut_host, tm_info->tm_hour, tm_info->tm_min);
                        }
                    }
                    response[strlen(response) - 1] = '\0';
                    endutent();

                    } else {
                        strcpy(response, "Nu sunteti autentificati.");
                    }
                    close(socket[0]);

                    if( write(socket[1], response, strlen(response)) < 0 ) {
                        printf("Eroare la scrierea socketpair.\n");
                        exit(1);
                    }

                    close(socket[1]);
                    exit(0);

            default:
                    int fifo_sc;
                    if( (fifo_sc = open( "server-client", O_WRONLY | O_TRUNC)) < 0 ) {
                        printf("Eroare la deschiderea fifo.\n");
                        exit(1);
                    }
                    if( read(socket[0], response, 1024) < 0 ) {
                        printf("Eroare la citirea din  canalul anonim.\n");
                        exit(1);
                    }
                    close(socket[0]);
                    if( write(fifo_sc, response, strlen(response)) < 0 ) {
                        printf("Eroare la scrierea in fifo.\n");
                        exit(1);
                    }
                    close(fifo_sc);

        }

    } else if( strcmp(command, "quit") == 0 ) {

//QUIT

        new_stat(0);
        unlink("client-server");
        int fifo_sc;
        if( (fifo_sc = open( "server-client", O_WRONLY | O_TRUNC)) < 0 ) {
            printf("Eroare la deschiderea fifo.\n");
            exit(1);
        }
        strcpy(response, "quit");
        if( write(fifo_sc, response, strlen(response)) < 0 ) {
            printf("Eroare la scrierea in fifo.\n");
            exit(1);
        }
        close(fifo_sc);

    } else if( strncmp(command, "get-proc-info : ", 16) == 0 ) {

//GET-PROC-INFO : PID

        switch( fork() ) {

            case -1:
                    printf("Eroare fork().\n");
                    exit(1);
                
            case 0: 
                    if( check_stat() ){
                        
                    char path[32] = "/proc/", line[50];
                    strcat(path, command+16);
                    strcat(path, "/status");

                    FILE* file = fopen(path, "r");
                    if ( file == NULL ) {
                        strcpy(response, "Nu exista proces cu astfel de pid.");
                    } else {
                        while(fgets(line, sizeof(line), file)){
                        if( strstr(line, "Name:") || strstr(line, "PPid:") || strstr(line, "State:") || strstr(line, "Uid:") || strstr(line, "VmSize:"))
                            sprintf(response + strlen(response), "%s", line);
                        }
                        response[strlen(response) - 1] = '\0';
                    }

                    } else {
                        strcpy(response, "Nu sunteti autentificati.");
                    }
                    close(socket[0]);

                    if( write(socket[1], response, strlen(response)) < 0 ) {
                        printf("Eroare la scrierea socketpair.\n");
                        exit(1);
                    }

                    close(socket[1]);
                    exit(0);

            default:
                    int fifo_sc;
                    if( (fifo_sc = open( "server-client", O_WRONLY | O_TRUNC)) < 0 ) {
                        printf("Eroare la deschiderea fifo.\n");
                        exit(1);
                    }
                    if( read(socket[0], response, 1024) < 0 ) {
                        printf("Eroare la citirea din  canalul anonim.\n");
                        exit(1);
                    }
                    close(socket[0]);
                    if( write(fifo_sc, response, strlen(response)) < 0 ) {
                        printf("Eroare la scrierea in fifo.\n");
                        exit(1);
                    }
                    close(fifo_sc);

        }
        

    } else {
        int fifo_sc;
        if( (fifo_sc = open( "server-client", O_WRONLY | O_TRUNC)) < 0 ) {
            printf("Eroare la deschiderea fifo.\n");
            exit(1);
        }

        strcpy(response, "Sintaxa gresita.");
        
        if( write(fifo_sc, response, strlen(response)) < 0 ) {
            printf("Eroare la scrierea in fifo.\n");
            exit(1);
        }
        close(fifo_sc);
    }

 }

}
             
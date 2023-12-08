#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

extern int errno;

int port;

int main(int argc, char *argv[])
{
  int sd;
  struct sockaddr_in server;
  char msg[100];

  if (argc != 3)
  {
    printf("[client] Sintaxa: %s <adresa_server> <port>\n", argv[0]);
    return -1;
  }

  port = atoi(argv[2]);

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("[client] Eroare la socket().\n");
    return errno;
  }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr(argv[1]);
  server.sin_port = htons(port);

  if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[client]Eroare la connect().\n");
    return errno;
  }

  while (1)
  {

    bzero(msg, 100);

    if (read(0, msg, 100) < 0)
    {
      perror("[client]Eroare la read() din stdin.\n");
      return errno;
    }

    while (msg[0] == ' ' || msg[0] == '\t' || msg[0] == '\n')
    {
      printf("Date invalide. Incercati din nou.\n");
      read(0, msg, 100);
    }

    if (write(sd, msg, strlen(msg) - 1) <= 0)
    {
      perror("[client]Eroare la write() spre server.\n");
      return errno;
    }

    if (read(sd, msg, 100) <= 0)
    {
      perror("[client]Eroare la read() de la server.\n");
      return errno;
    }

    printf("%s\n", msg);
    fflush(stdout);

   /* if (strcmp(msg, "Pregatire chat.") == 0)
    {
      fd_set readfds;
      int nfds;

      while (1)
      {
        FD_ZERO(&readfds);
        FD_SET(sd, &readfds);
        FD_SET(0, &readfds);
        nfds = sd;

        if (select(nfds + 1, &readfds, NULL, NULL, NULL) < 0)
        {
          perror("[client]Eroare la select().\n");
          return errno;
        }

        // Verifică dacă este activitate pe socket-ul serverului
        if (FD_ISSET(sd, &readfds))
        {
          bzero(msg, 100);

          if (read(sd, msg, 100) <= 0)
          {
            printf("Server deconectat.\n");
            break;
          }
          printf("Mesaj primit: %s", msg);
        }

        if (FD_ISSET(0, &readfds))
        {
          bzero(msg, 100);
          read(0, msg, 100);

          

          if (write(sd, msg, strlen(msg)) < 0)
          {
            perror("ERROR writing to socket");
            break;
          }

          if (strcmp(msg, "stop-chat") == 0)
          {
            break;
          }
        }
      }
    }
    */
   
  }


  close(sd);
}

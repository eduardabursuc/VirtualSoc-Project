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
#include <termios.h>
#include <unistd.h>

extern int errno;

int port;

void disable_echo()
{
  struct termios t;
  tcgetattr(STDIN_FILENO, &t);
  t.c_lflag &= ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

// Function to enable terminal echo
void enable_echo()
{
  struct termios t;
  tcgetattr(STDIN_FILENO, &t);
  t.c_lflag |= ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

int main(int argc, char *argv[])
{
  int sd;
  struct sockaddr_in server;
  char msg[1000];
  fd_set readfds;
  int nfds;

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
    FD_ZERO(&readfds);
    FD_SET(sd, &readfds);
    FD_SET(0, &readfds);
    nfds = sd;

    if (select(nfds + 1, &readfds, NULL, NULL, NULL) < 0)
    {
      perror("[client]Eroare la select().\n");
      return errno;
    }

    if (FD_ISSET(sd, &readfds))
    {
      bzero(msg, 1000);

      if (read(sd, msg, 1000) <= 0)
      {
        printf("Server deconectat.\n");
        break;
      }
      if (strncmp(msg, "Parola", 6) == 0)
      {
        char answer[1001];
        strcpy(answer, msg);
        strcat(answer, "\n");
        bzero(msg, 1000);
        strcpy(msg, getpass(answer));
        write(sd, msg, strlen(msg));
      }
      else
      {
        printf("%s\n", msg);
        fflush(stdout);
      }
    }

    if (FD_ISSET(0, &readfds))
    {

      bzero(msg, 1000);
      read(0, msg, 1000);

      while (msg[0] == ' ' || msg[0] == '\t' || msg[0] == '\n')
      {
        printf("Date invalide. Incercati din nou.\n");
        read(0, msg, 100);
      }

      if (write(sd, msg, strlen(msg) - 1) < 0)
      {
        perror("ERROR writing to socket");
        break;
      }
    }
  }

  close(sd);
}

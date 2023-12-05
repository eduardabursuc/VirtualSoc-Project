#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <sqlite3.h>

#define PORT 2222

extern int errno;
char answ[1000];

struct User {
  int fd;
  char username[101];
};

// DATA TABLES

sqlite3 *db;
char *errMsg = 0;

const char *createUsers = "CREATE TABLE IF NOT EXISTS users ("
                          "    username TEXT PRIMARY KEY,"
                          "    password TEXT,"
                          "    type_of_user TEXT,"
                          "    type_of_profile TEXT"
                          ");";

const char *createFriends = "CREATE TABLE IF NOT EXISTS friends ("
                            "    host_username TEXT PRIMARY KEY,"
                            "    friend_username TEXT,"
                            "    type_of_friend TEXT"
                            ");";

const char *createPosts = "CREATE TABLE IF NOT EXISTS posts ("
                          "    username TEXT PRIMARY KEY,"
                          "    public_posts TEXT,"
                          "    close_friend_posts TEXT,"
                          "    friend_posts TEXT"
                          ");";

int createTables()
{
  char *errMsg = 0;

  if (sqlite3_exec(db, createUsers, 0, 0, &errMsg) != SQLITE_OK)
  {
    fprintf(stderr, "SQL error: %s\n", errMsg);
    sqlite3_free(errMsg);
    return 1;
  }

  if (sqlite3_exec(db, createFriends, 0, 0, &errMsg) != SQLITE_OK)
  {
    fprintf(stderr, "SQL error: %s\n", errMsg);
    sqlite3_free(errMsg);
    return 1;
  }

  if (sqlite3_exec(db, createPosts, 0, 0, &errMsg) != SQLITE_OK)
  {
    fprintf(stderr, "SQL error: %s\n", errMsg);
    sqlite3_free(errMsg);
    return 1;
  }

  return 0;
}

// REGISTRATION

void regist(int fd)
{

  char username[100] = "", passwd[100] = "", type[10] = "";

  const char *selectDataSQL = "SELECT * FROM users WHERE username = ?;";
  sqlite3_stmt *statement;

  strcpy(answ, "Doriti sa creati un cont de administrator sau client? (admin/user)");
  write(fd, answ, strlen(answ));
  read(fd, type, 10);
  type[strlen(type) - 1] = '\0';

  strcpy(answ, "Introduceti un username: ");
  write(fd, answ, strlen(answ));
  read(fd, username, 100);
  username[strlen(username) - 1] = '\0';
  printf("%s\n", username);

  if (sqlite3_prepare_v2(db, selectDataSQL, -1, &statement, 0) == SQLITE_OK)
  {
    sqlite3_bind_text(statement, 1, username, -1, SQLITE_STATIC);
    int selectResult = sqlite3_step(statement);

    if (selectResult != SQLITE_DONE)
    {
      strcpy(answ, "Cont existent.");
      write(fd, answ, strlen(answ));
    }
    else
    {
      strcpy(answ, "Introduceti o parola: ");
      write(fd, answ, strlen(answ));
      read(fd, passwd, 100);
      passwd[strlen(passwd) - 1] = '\0';

      fflush(stdout);
      const char *selectDataSQL = "insert into users values (?, ?, ?, NULL);";
      if (sqlite3_prepare_v2(db, selectDataSQL, -1, &statement, 0) == SQLITE_OK)
      {
        sqlite3_bind_text(statement, 1, username, -1, SQLITE_STATIC);
        sqlite3_bind_text(statement, 2, passwd, -1, SQLITE_STATIC);
        sqlite3_bind_text(statement, 3, type, -1, SQLITE_STATIC);

        if (sqlite3_step(statement) != SQLITE_DONE)
        {
          // Handle the insertion error
        }
      }
    }
  }
  else
  {
    // error handler
  }
}

// LOGIN

void login(struct User user)
{

  // sqlite part : cautare in tabela users

  // daca gasim usernameul:

  char passwd[100];
  printf("Parola: ");
  read(0, passwd, 100);
  passwd[strlen(passwd)] = '\0';

  // comparam cu parola din tabel
}

// SHOW-USERS

// ADD-FRIEND

// NEW-POST

// SHOW-PROFILE

// CHAT

// LOGOUT

char *conv_addr(struct sockaddr_in address)
{
  static char str[25];
  char port[7];

  strcpy(str, inet_ntoa(address.sin_addr));
  bzero(port, 7);
  sprintf(port, ":%d", ntohs(address.sin_port));
  strcat(str, port);
  return (str);
}

int main()
{
  struct sockaddr_in server;
  struct sockaddr_in from;
  fd_set readfds;
  fd_set actfds;
  struct timeval tv;
  int sd, client;
  int optval = 1;
  int nfds;
  int len;
  struct User user;
  char command[1000];

  if (sqlite3_open("data.db", &db) != SQLITE_OK)
  {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
    return 1;
  }

  if (createTables())
  {
    perror("[server] Eroare la createTables().\n");
    return errno;
  }

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("[server] Eroare la socket().\n");
    return errno;
  }

  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

  bzero(&server, sizeof(server));

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(PORT);

  if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[server] Eroare la bind().\n");
    return errno;
  }

  if (listen(sd, 5) == -1)
  {
    perror("[server] Eroare la listen().\n");
    return errno;
  }

  FD_ZERO(&actfds);
  FD_SET(sd, &actfds);

  tv.tv_sec = 1;
  tv.tv_usec = 0;

  nfds = sd;

  printf("[server] Asteptam la portul %d...\n", PORT);
  fflush(stdout);

  while (1)
  {

    bcopy((char *)&actfds, (char *)&readfds, sizeof(readfds));

    if (select(nfds + 1, &readfds, NULL, NULL, &tv) < 0)
    {
      perror("[server] Eroare la select().\n");
      return errno;
    }

    if (FD_ISSET(sd, &readfds))
    {
      len = sizeof(from);
      bzero(&from, sizeof(from));

      client = accept(sd, (struct sockaddr *)&from, &len);

      if (client < 0)
      {
        perror("[server] Eroare la accept().\n");
        continue;
      }

      if (nfds < client)
        nfds = client;

      FD_SET(client, &actfds);

      printf("[server] S-a conectat clientul cu descriptorul %d, de la adresa %s.\n", client, conv_addr(from));
      fflush(stdout);
    }

    for (user.fd = 0; user.fd <= nfds; user.fd++)
    {
      if (user.fd != sd && FD_ISSET(user.fd, &readfds))
      {

        bzero(command, 1000);
        bzero(answ, sizeof(answ));

        ssize_t bytes_received = recv(user.fd, command, sizeof(command), 0);

        if (bytes_received <= 0)
        {
          close(user.fd);
          FD_CLR(user.fd, &actfds);
          printf("[server] Client with descriptor %d disconnected.\n", user.fd);
          continue;
        }

        command[strlen(command) - 1] = '\0';

        if (strncmp(command, "registration", 12) == 0)
        {
          regist(user.fd);
          // strcpy(answ, "Cerere de registrare.");
          // write(fd, answ, strlen(answ));
        }
        else if (strcmp(command, "login") == 0)
        {
          // login(user);
          strcpy(answ, "Cerere de logare.");
          write(user.fd, answ, strlen(answ));
        }
        else if (strcmp(command, "logout") == 0)
        {
          strcpy(answ, "Cerere de iesire din cont.");
          write(user.fd, answ, strlen(answ));
        }
        else if (strncmp(command, "add-friend : ", 13) == 0)
        {

          strcpy(answ, "Cerere de adaugeare prieten.");
          write(user.fd, answ, strlen(answ));
        }
        else if (strcmp(command, "new-post") == 0)
        {

          strcpy(answ, "Cerere de postare a unei noutati.");
          write(user.fd, answ, strlen(answ));
        }
        else if (strcmp(command, "show-users") == 0)
        {

          strcpy(answ, "Cerere de afisare utilizatori.");
          write(user.fd, answ, strlen(answ));
        }
        else if (strncmp(command, "chat : ", 7) == 0)
        {

          strcpy(answ, "Cerere de creare chat.");
          write(user.fd, answ, strlen(answ));
        }
        else
        {

          strcpy(answ, "Comanda necunoscuta.");
          write(user.fd, answ, strlen(answ));
        }
      }
    }
  }
}

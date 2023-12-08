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

struct User
{
  int fd;
  char username[101];
};

// DATA TABLES

sqlite3 *db;
char *errMsg = 0;

const char *createUsers = "CREATE TABLE IF NOT EXISTS users ("
                          "    username TEXT PRIMARY KEY,"
                          "    password TEXT NOT NULL,"
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

int userExists(char username[])
{
  const char *selectDataSQL = "select * from users where username = ?;";
  sqlite3_stmt *statement;
  if (sqlite3_prepare_v2(db, selectDataSQL, -1, &statement, 0) == SQLITE_OK)
  {
    sqlite3_bind_text(statement, 1, username, -1, SQLITE_STATIC);
    int selectResult = sqlite3_step(statement);

    if (selectResult != SQLITE_DONE)
    {

      return 1;
    }
    else
    {

      return 0;
    }
  }
  else
  {
    return -1;
  }
}

// FUNCTIONS

// REGISTRATION

void registration(int fd)
{

  char username[100] = "", passwd[100] = "", type_acc[10] = "", type_usr[10] = "";

  strcpy(answ, "Doriti sa creati un cont de administrator sau client? (admin/user)");
  write(fd, answ, strlen(answ));
  read(fd, type_usr, 10);

  while (strcmp(type_usr, "admin") != 0 && strcmp(type_usr, "user") != 0)
  {
    bzero(type_usr, 10);
    strcpy(answ, "Introduceti un cuvant cheie admin/user.");
    write(fd, answ, strlen(answ));
    read(fd, type_usr, 10);
  }

  strcpy(answ, "Doriti sa creati un cont public sau privat? (public/privat)");
  write(fd, answ, strlen(answ));
  read(fd, type_acc, 10);

  while (strcmp(type_acc, "public") != 0 && strcmp(type_acc, "privat") != 0)
  {
    bzero(type_acc, 10);
    strcpy(answ, "Introduceti un cuvant cheie public/privat.");
    write(fd, answ, strlen(answ));
    read(fd, type_acc, 10);
  }

  strcpy(answ, "Introduceti un username: ");
  write(fd, answ, strlen(answ));
  read(fd, username, 100);

  int exists = userExists(username);

  if (exists == 1)
  {
    strcpy(answ, "Cont existent.");
    write(fd, answ, strlen(answ));
  }
  else if (exists == 0)
  {
    strcpy(answ, "Introduceti o parola: ");
    write(fd, answ, strlen(answ));
    read(fd, passwd, 100);
    sqlite3_stmt *statement;

    const char *selectDataSQL = "insert into users values (?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, selectDataSQL, -1, &statement, 0) == SQLITE_OK)
    {
      sqlite3_bind_text(statement, 1, username, -1, SQLITE_STATIC);
      sqlite3_bind_text(statement, 2, passwd, -1, SQLITE_STATIC);
      sqlite3_bind_text(statement, 3, type_usr, -1, SQLITE_STATIC);
      sqlite3_bind_text(statement, 4, type_acc, -1, SQLITE_STATIC);

      if (sqlite3_step(statement) != SQLITE_DONE)
      {
        strcpy(answ, "Datele introduse sunt invalide.");
        write(fd, answ, strlen(answ));
      }
      else
      {
        strcpy(answ, "Inregistrare cu succes.");
        write(fd, answ, strlen(answ));
      }
    }
  }
  else
  {
    strcpy(answ, "Eroare la cautarea numelui de utilizator in baza de date.");
    write(fd, answ, strlen(answ));
  }
}

// LOGIN

void login(struct User *user)
{
  char username[100] = "", passwd[100] = "";

  printf("fd: %d, username: %s\n", user->fd, user->username);

  if (strlen(user->username) > 0)
  {
    strcpy(answ, "Sunteti autentificat deja.");
    write(user->fd, answ, strlen(answ));
  } else {

  strcpy(answ, "Username:");
  write(user->fd, answ, strlen(answ));
  read(user->fd, username, 10);

  const char *selectDataSQL = "select password from users where username = ?;";
  sqlite3_stmt *statement;

  if (sqlite3_prepare_v2(db, selectDataSQL, -1, &statement, 0) == SQLITE_OK)
  {
    sqlite3_bind_text(statement, 1, username, -1, SQLITE_STATIC);

    int selectResult = sqlite3_step(statement);

    if (selectResult == SQLITE_ROW)
    {
      const char *password = (const char *)sqlite3_column_text(statement, 0);
      printf("Password: %s\n", password);

      int times = 0;
      while (strcmp(passwd, password) != 0)
      {
        if (times == 0)
        {
          times = 1;
          strcpy(answ, "Parola:");
        }
        else
          strcpy(answ, "Parola gresita.\nIntroduceti alta parola:");
        fflush(stdout);
        write(user->fd, answ, strlen(answ));
        read(user->fd, passwd, 100);
      }

      strcpy(user->username, username);
      strcpy(answ, "Logare cu succes.");
      write(user->fd, answ, strlen(answ));
    }
    else if (selectResult == SQLITE_DONE)
    {
      strcpy(answ, "Utilizatorul nu a fost gasit.");
      write(user->fd, answ, strlen(answ));
    }
    else
    {
      strcpy(answ, "Eroare la extragerea datelor.");
      write(user->fd, answ, strlen(answ));
    }
    sqlite3_finalize(statement);
  }
  }
}

// SHOW-USERS

void show_users(int fd) 
{
  const char *selectDataSQL = "select username from users;";
  sqlite3_stmt *statement;

  int prepareResult = sqlite3_prepare_v2(db, selectDataSQL, -1, &statement, NULL);

  if (prepareResult != SQLITE_OK) {
      fprintf(stderr, "Failed to prepare SQL statement: %s\n", sqlite3_errmsg(db));
      return;
  }

  strcpy(answ, "");
    
  while (sqlite3_step(statement) == SQLITE_ROW) 
  {
    const char *username = (const char *)sqlite3_column_text(statement, 0);
    strcat(answ, username);
    strcat(answ, "\n");
  }

  write(fd, answ, strlen(answ) - 1);

}


// ADD-FRIEND

// NEW-POST

// SHOW-PROFILE

// CHAT
/*
void chat(int sd, fd_set& readfds, fd_set& actfds, int fd[], int n) {
    
    for ( int i = 0; i < n; i++ ) {

      strcpy(answ, "Pregatire chat.");
      write(fd[i], answ, strlen(answ));

        if (fd != sd && FD_ISSET(fd[i], &readfds)) {
            char message[1024];
            bzero(message, 1024);
            ssize_t bytes_received = recv(fd[i], message, sizeof(message), 0);

            if (bytes_received <= 0) {
                continue;
            }

            printf("[server] Client: %s\n", message);

            for (int f = 0; f <= nfds; f++) {
                if (f != sd && FD_ISSET(f, &actfds) && f != fd) {
                    if (write(f, message, bytes_received) < 0) {
                        perror("[server] Error writing to client.\n");
                    }
                }
            }
        }
    }
}
*/

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
  int client, sd;
  int optval = 1;
  int nfds;
  int len;
  struct User users[10001];
  int i = 0, j;
  char command[1000];

  for (int j = 0; j < 10001; j++)
    strcpy(users[j].username, "");

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
      users[i].fd = client;
      i++;

      printf("[server] S-a conectat clientul cu descriptorul %d, de la adresa %s.\n", client, conv_addr(from));
      fflush(stdout);
    }

    for (int fd = 0; fd <= nfds; fd++)
    {
      if (fd != sd && FD_ISSET(fd, &readfds))
      {

        bzero(command, 1000);
        bzero(answ, sizeof(answ));

        ssize_t bytes_received = recv(fd, command, sizeof(command), 0);

        if (bytes_received <= 0)
        {
          for( j = 0; j < i; j++ ){
            if( fd == users[j].fd ) {
              users[j] = users[i-1];
              i--;
              break;
            }
          }
          close(fd);
          FD_CLR(fd, &actfds);
          printf("[server] Client with descriptor %d disconnected.\n", fd);
          continue;
        }

        if (strcmp(command, "registration") == 0)
        {
          for (j = 0; j < i; j++)
          {
            if (users[j].fd == fd)
            {
              if (strlen(users[j].username) > 0)
              {
                strcpy(answ, "Sunteti autentificat.");
                write(fd, answ, strlen(answ));
              }
              else
              {
                registration(fd);
              }
            }
          }
          // strcpy(answ, "Cerere de registrare.");
        }
        else if (strcmp(command, "login") == 0)
        {
          for (j = 0; j < i; j++)
          {
            if (users[j].fd == fd)
            break;
          }
          login(&users[j]);
          // strcpy(answ, "Cerere de logare.");
        }
        else if (strcmp(command, "logout") == 0)
        {
          if (users[j].fd == fd)
          {

            if (strlen(users[j].username) > 0)
            {
              strcpy(users[j].username, "");
              strcpy(answ, "Logout cu succes.");
              write(fd, answ, strlen(answ));
            }
            else
            {
              strcpy(answ, "Nu sunteti autentificat.");
              write(fd, answ, strlen(answ));
            }
          }
          // strcpy(answ, "Cerere de iesire din cont.");
        }
        else if (strncmp(command, "add-friend : ", 13) == 0)
        {
          strcpy(answ, "Cerere de adaugeare prieten.");
          write(fd, answ, strlen(answ));
        }
        else if (strcmp(command, "new-post") == 0)
        {
          strcpy(answ, "Cerere de postare a unei noutati.");
          write(fd, answ, strlen(answ));
        }
        else if (strcmp(command, "show-users") == 0)
        {
          //strcpy(answ, "Cerere de afisare utilizatori.");
          //write(fd, answ, strlen(answ));
          show_users(fd);
        }
        else if (strncmp(command, "chat : ", 7) == 0)
        {
          //strcpy(answ, "Cerere de creare chat.");
          //write(fd, answ, strlen(answ));
          char usernames[999];
          strcpy(usernames, command+7);
          char *token = strtok(usernames, " ");
          int fds[999], k = 0;

          while (token != NULL) {
             for( j = 0; j < i; j++ ){
              printf("%s\n", token);
              if(strcmp(users[j].username, token) == 0 ){
                fds[k] = users[j].fd;
                k++;
                printf("%d : %d\n", k, fds[k]);
              }
             }
             token = strtok(NULL, " ");
             
          }
          fds[k] = fd;

        }
        else if (strncmp(command, "show-profile : ", 15) == 0)
        {
          strcpy(answ, "Cerere de vizualizare profil.");
          write(fd, answ, strlen(answ));
        }
        else
        {

          strcpy(answ, "Comanda necunoscuta.");
          write(fd, answ, strlen(answ));
        }

        
      }
    }
  }
}

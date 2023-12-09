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

    const char *insertDataSQL = "insert into users values (?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, insertDataSQL, -1, &statement, 0) == SQLITE_OK)
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
  }
  else
  {

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

  if (prepareResult != SQLITE_OK)
  {
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

void add_friend(struct User user, char friend[])
{
  char relationship[31] = "";

  strcpy(answ, "Introduceti tipul de relatie (friend/close-friend):");
  write(user.fd, answ, strlen(answ));
  read(user.fd, relationship, 31);

  while (strcmp(relationship, "friend") != 0 && strcmp(relationship, "close-friend") != 0)
  {
    strcpy(answ, "Introduceti doar cuvantul cheie (friend/close-friend):");
    write(user.fd, answ, strlen(answ));
    read(user.fd, relationship, 31);
  }

  const char *updateDataSQL = "update friends set type_of_friend = ? where host_username = ? and friend_username = ?;";
  const char *selectDataSQL = "select * from friends where host_username = ? and friend_username = ?;";
  sqlite3_stmt *statement;

  if (sqlite3_prepare_v2(db, selectDataSQL, -1, &statement, 0) == SQLITE_OK)
  {
    sqlite3_bind_text(statement, 1, user.username, -1, SQLITE_STATIC);
    sqlite3_bind_text(statement, 2, friend, -1, SQLITE_STATIC);

    if (sqlite3_step(statement) != SQLITE_DONE)
    {
      if (sqlite3_prepare_v2(db, updateDataSQL, -1, &statement, 0) == SQLITE_OK)
      {
        sqlite3_bind_text(statement, 1, relationship, -1, SQLITE_STATIC);
        sqlite3_bind_text(statement, 2, user.username, -1, SQLITE_STATIC);
        sqlite3_bind_text(statement, 3, friend, -1, SQLITE_STATIC);

        if (sqlite3_step(statement) == SQLITE_DONE)
        {
          strcpy(answ, "Modificat cu succes.");
          write(user.fd, answ, strlen(answ));
        }
        else
        {
          fprintf(stderr, "SQL error during update: %s\n", sqlite3_errmsg(db));
          strcpy(answ, "Eroare la introducerea datelor.");
          write(user.fd, answ, strlen(answ));
        }
        sqlite3_finalize(statement);
      }
    }
    else
    {
      sqlite3_prepare_v2(db, selectDataSQL, -1, &statement, 0);
      sqlite3_bind_text(statement, 1, friend, -1, SQLITE_STATIC);
      sqlite3_bind_text(statement, 2, user.username, -1, SQLITE_STATIC);

      if (sqlite3_step(statement) != SQLITE_DONE)
      {
        if (sqlite3_prepare_v2(db, updateDataSQL, -1, &statement, 0) == SQLITE_OK)
        {
          sqlite3_bind_text(statement, 1, relationship, -1, SQLITE_STATIC);
          sqlite3_bind_text(statement, 2, friend, -1, SQLITE_STATIC);
          sqlite3_bind_text(statement, 3, user.username, -1, SQLITE_STATIC);

          if (sqlite3_step(statement) == SQLITE_DONE)
          {
            strcpy(answ, "Modificat cu succes.");
            write(user.fd, answ, strlen(answ));
          }
          else
          {
            fprintf(stderr, "SQL error during update: %s\n", sqlite3_errmsg(db));
            strcpy(answ, "Eroare la introducerea datelor.");
            write(user.fd, answ, strlen(answ));
          }

          sqlite3_finalize(statement);
        }
      }
      else
      {
        const char *insertDataSQL = "insert into friends values (?, ?, ?);";
        if (sqlite3_prepare_v2(db, insertDataSQL, -1, &statement, 0) == SQLITE_OK)
        {
          sqlite3_bind_text(statement, 1, user.username, -1, SQLITE_STATIC);
          sqlite3_bind_text(statement, 2, friend, -1, SQLITE_STATIC);
          sqlite3_bind_text(statement, 3, relationship, -1, SQLITE_STATIC);

          if (sqlite3_step(statement) != SQLITE_DONE)
          {
            strcpy(answ, "Datele introduse sunt invalide.");
            printf("%s %s %s\n", user.username, friend, relationship);
            write(user.fd, answ, strlen(answ));
          }
          else
          {
            strcpy(answ, "Inregistrare cu succes.");
            write(user.fd, answ, strlen(answ));
          }
          sqlite3_finalize(statement);
        }
      }
    }
  }
  else
  {
    strcpy(answ, "Eroare.");
    write(user.fd, answ, strlen(answ));
  }
}

// NEW-POST

// SHOW-PROFILE

// CHAT

void chat(struct User users[], int n)
{

  char message[1001];
  bzero(message, 1001);

  for (int i = 1; i <= n; i++)
  {
    strcpy(answ, "Pregatire chat.");
    write(users[i].fd, answ, strlen(answ));
  }

  while (n != 1)
  {
    fd_set readfds;
    int nfds = users[0].fd;
    FD_ZERO(&readfds);

    for (int i = 1; i <= n; i++)
    {
      FD_SET(users[i].fd, &readfds);
      if (nfds < users[i].fd)
        nfds = users[i].fd;
    }

    if (select(nfds + 1, &readfds, NULL, NULL, NULL) < 0)
    {
      perror("[server]Eroare la select().\n");
    }

    for (int i = 1; i <= n; i++)
    {
      if (FD_ISSET(users[i].fd, &readfds))
      {
        bzero(message, 1001);
        if (read(users[i].fd, message, 1001) <= 0 || strcmp(message, "stop-chat") == 0)
        {

          strcpy(answ, "Ati iesit din chat.");
          write(users[i].fd, answ, strlen(answ));
          users[i] = users[n];
          n--;
          if (n == 1)
          {
            strcpy(answ, "Chat inchis.");
            write(users[i].fd, answ, strlen(answ));
          }
          printf("Server deconectat.\n");
          break;
        }
        else
        {
          for (int j = 1; j <= n; j++)
          {
            if (users[i].fd != users[j].fd)
            {
              char aux[1200];
              bzero(aux, 1200);
              strcpy(aux, users[i].username);
              strcat(aux, " : ");
              strcat(aux, message);
              write(users[j].fd, aux, strlen(aux));
            }
          }
        }
      }
    }
  }
}

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
          for (j = 0; j < i; j++)
          {
            if (fd == users[j].fd)
            {
              users[j] = users[i - 1];
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
              break;
            }
          }
          // strcpy(answ, "Cerere de registrare.");
          // write(fd, answ, strlen(answ));
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
          // write(fd, answ, strlen(answ));
        }
        else if (strcmp(command, "logout") == 0)
        {
          for (j = 0; j < i; j++)
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
              break;
            }
          }

          // strcpy(answ, "Cerere de iesire din cont.");
          // write(fd, answ, strlen(answ));
        }
        else if (strncmp(command, "add-friend : ", 13) == 0)
        {
          // strcpy(answ, "Cerere de adaugeare prieten.");
          // write(fd, answ, strlen(answ));
          for (j = 0; j < i; j++)
          {
            if (users[j].fd == fd)
              break;
          }
          if (strlen(users[j].username) > 0 && userExists(command + 13) == 1)
          {
            if (strcmp(users[j].username, command + 13) == 0)
            {
              strcpy(answ, "Nu puteti sa va adaugati ca prieten propriu.");
              write(fd, answ, strlen(answ));
            }
            else
            {
              add_friend(users[j], command + 13);
            }
          }
          else
          {
            strcpy(answ, "Nu sunteti autentificati sau ati introdus un cont inexistent.");
            write(fd, answ, strlen(answ));
          }
        }
        else if (strcmp(command, "new-post") == 0)
        {
          strcpy(answ, "Cerere de postare a unei noutati.");
          write(fd, answ, strlen(answ));
        }
        else if (strcmp(command, "show-users") == 0)
        {
          // strcpy(answ, "Cerere de afisare utilizatori.");
          // write(fd, answ, strlen(answ));
          show_users(fd);
        }
        else if (strncmp(command, "chat : ", 7) == 0)
        {
          // strcpy(answ, "Cerere de creare chat.");
          // write(fd, answ, strlen(answ));
          for (j = 0; j < i; j++)
          {
            if (users[j].fd == fd)
            {
              if (strlen(users[j].username) > 0)
              {
                char *token = strtok(command + 7, " ");
                int k = 2;
                struct User a_users[999];
                a_users[1] = users[j];

                while (token != NULL)
                {
                  printf("%s\n", token);

                  for (int q = 0; q < i; q++)
                  {
                    if (strcmp(users[q].username, token) == 0 && q != j)
                    {
                      a_users[k] = users[q];
                      k++;
                      printf("%d : %d\n", k - 1, a_users[k - 1].fd);
                    }
                  }
                  token = strtok(NULL, " ");
                }
                if (k - 1 > 1)
                {
                  chat(a_users, k - 1);
                }
                else
                {
                  strcpy(answ, "Din pacate utilizatorii mentionati nu sunt activi.");
                  write(fd, answ, strlen(answ));
                }
              }
              else
              {
                strcpy(answ, "Nu sunteti autentificat.");
                write(fd, answ, strlen(answ));
              }
              break;
            }
          }
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

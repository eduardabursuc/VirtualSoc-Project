#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sqlite3.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>

#define PORT 2222

pthread_mutex_t lock;
char answ[1001];

struct User
{
  int fd;
  char username[101];
  int chat_request;
  int chat_accepted;
} users[10001];

int client_index = 0;
sqlite3 *db;
char *errMsg = 0;
extern int errno;

typedef struct thData
{
  int idThread;
  int cl;
} thData;

const char *createUsers = "CREATE TABLE IF NOT EXISTS users ("
                          "    username TEXT PRIMARY KEY,"
                          "    password TEXT NOT NULL,"
                          "    type_of_user TEXT,"
                          "    type_of_profile TEXT"
                          ");";

const char *createFriends = "CREATE TABLE IF NOT EXISTS friends ("
                            "    host_username TEXT,"
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

struct User *userByFd( int fd ) {
  for( int i = 0 ; i < client_index; i++ ){
    if( fd == users[i].fd ) return &users[i];
  }
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

int checkFriends(char host[], char friend[])
{
  const char *selectDataSQL = "SELECT * FROM friends WHERE host_username = ? AND friend_username = ?;";
  sqlite3_stmt *statement;

  if (sqlite3_prepare_v2(db, selectDataSQL, -1, &statement, 0) != SQLITE_OK)
  {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
    return 2;
  }

  sqlite3_bind_text(statement, 1, host, -1, SQLITE_STATIC);
  sqlite3_bind_text(statement, 2, friend, -1, SQLITE_STATIC);

  int stepResult = sqlite3_step(statement);

  if (stepResult != SQLITE_DONE)
  {
    sqlite3_finalize(statement);
    return 1;
  }
  else
  {
    sqlite3_finalize(statement);
    return 0;
  }
}

// FUNCTIONS

// REGISTRATION

void registration(int fd)
{

  char username[100] = "", passwd[100] = "", type_acc[10] = "", type_usr[10] = "";

  strcpy(answ, "Doriti sa creati un cont de administrator sau client? (admin/user)\n");
  write(fd, answ, strlen(answ));
  read(fd, type_usr, 10);

  while (strcmp(type_usr, "admin") != 0 && strcmp(type_usr, "user") != 0)
  {
    bzero(type_usr, 10);
    strcpy(answ, "Introduceti un cuvant cheie admin/user.\n");
    write(fd, answ, strlen(answ));
    if (read(fd, type_usr, 10) <= 0 ) {
      printf("Client disconnected.\n");
      pthread_exit(NULL);
    }

  }

  strcpy(answ, "Doriti sa creati un cont public sau privat? (public/privat)\n");
  write(fd, answ, strlen(answ));
  read(fd, type_acc, 10);

  while (strcmp(type_acc, "public") != 0 && strcmp(type_acc, "privat") != 0)
  {
    bzero(type_acc, 10);
    strcpy(answ, "Introduceti un cuvant cheie public/privat.\n");
    write(fd, answ, strlen(answ));
    if (read(fd, type_acc, 10) <= 0 ){
      printf("Client disconnected.\n");
      pthread_exit(NULL);
    }
  }

  strcpy(answ, "Introduceti un username: \n");
  write(fd, answ, strlen(answ));
  read(fd, username, 100);

  int exists = userExists(username);

  if (exists == 1)
  {
    strcpy(answ, "Cont existent.\n");
    write(fd, answ, strlen(answ));
  }
  else if (exists == 0)
  {
    strcpy(answ, "Parola dorita: \n");
    write(fd, answ, strlen(answ));
    read(fd, passwd, 100);
    sqlite3_stmt *statement;

    pthread_mutex_lock(&lock);

    const char *insertDataSQL = "insert into users values (?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, insertDataSQL, -1, &statement, 0) == SQLITE_OK)
    {
      sqlite3_bind_text(statement, 1, username, -1, SQLITE_STATIC);
      sqlite3_bind_text(statement, 2, passwd, -1, SQLITE_STATIC);
      sqlite3_bind_text(statement, 3, type_usr, -1, SQLITE_STATIC);
      sqlite3_bind_text(statement, 4, type_acc, -1, SQLITE_STATIC);

      if (sqlite3_step(statement) != SQLITE_DONE)
      {
        strcpy(answ, "Datele introduse sunt invalide.\n");
        write(fd, answ, strlen(answ));
      }
      else
      {
        strcpy(answ, "Inregistrare cu succes.\n");
        write(fd, answ, strlen(answ));
      }
    }
  }
  else
  {
    strcpy(answ, "Eroare la cautarea numelui de utilizator in baza de date.\n");
    write(fd, answ, strlen(answ));
  }

  pthread_mutex_unlock(&lock);

}

// LOGIN

void login(struct User *user)
{
  char username[100] = "", passwd[100] = "";

  printf("fd: %d, username: %s\n", user->fd, user->username);

  if (strlen(user->username) > 0)
  {
    strcpy(answ, "Sunteti autentificat deja.\n");
    write(user->fd, answ, strlen(answ));
  }
  else
  {

    strcpy(answ, "Username:\n");
    write(user->fd, answ, strlen(answ));
    read(user->fd, username, 100);

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
            strcpy(answ, "Parola:\n");
          }
          else
            strcpy(answ, "Parola gresita.\nIncercati o alta parola:\n");
          fflush(stdout);
          write(user->fd, answ, strlen(answ) - 1);
          if (read(user->fd, passwd, 100) <= 0){
            printf("Client disconnected.\n");
            pthread_exit(NULL);
          }
        }

        strcpy(user->username, username);
        strcpy(answ, "Logare cu succes.\n");
        write(user->fd, answ, strlen(answ));
      }
      else if (selectResult == SQLITE_DONE)
      {
        strcpy(answ, "Utilizatorul nu a fost gasit.\n");
        write(user->fd, answ, strlen(answ));
      }
      else
      {
        strcpy(answ, "Eroare la extragerea datelor.\n");
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

  write(fd, answ, strlen(answ));
}

// ADD-FRIEND

void add_friend(struct User user, char friend[])
{

  char relationship[31] = "";
  int a = 0;

  strcpy(answ, "Introduceti tipul de relatie (friend/close-friend):\n");
  write(user.fd, answ, strlen(answ));
  read(user.fd, relationship, 31);

  while (strcmp(relationship, "friend") != 0 && strcmp(relationship, "close-friend") != 0)
  {
    strcpy(answ, "Introduceti doar cuvantul cheie (friend/close-friend):\n");
    write(user.fd, answ, strlen(answ));
    if (read(user.fd, relationship, 31) <= 0) {
      printf("Client disconnected.\n");
      pthread_exit(NULL);
    }
  }

  const char *updateDataSQL = "UPDATE friends SET type_of_friend = ? WHERE host_username = ? AND friend_username = ?;";
  const char *insertDataSQL = "INSERT INTO friends VALUES (?, ?, ?);";

  sqlite3_stmt *statement;

  pthread_mutex_lock(&lock);

  if ((a = checkFriends(user.username, friend)) == 1 || checkFriends(friend, user.username) == 1)
  {
    // Record exists, perform update
    if (sqlite3_prepare_v2(db, updateDataSQL, -1, &statement, 0) != SQLITE_OK)
    {
      fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
      strcpy(answ, "Eroare la pregatirea interogarii de actualizare.\n");
      write(user.fd, answ, strlen(answ));
      return;
    }

    sqlite3_bind_text(statement, 1, relationship, -1, SQLITE_STATIC);
    if (a)
    {
      sqlite3_bind_text(statement, 2, user.username, -1, SQLITE_STATIC);
      sqlite3_bind_text(statement, 3, friend, -1, SQLITE_STATIC);
    }
    else
    {
      sqlite3_bind_text(statement, 2, friend, -1, SQLITE_STATIC);
      sqlite3_bind_text(statement, 3, user.username, -1, SQLITE_STATIC);
    }

    if (sqlite3_step(statement) == SQLITE_DONE)
    {
      strcpy(answ, "Modificat cu succes.\n");
      write(user.fd, answ, strlen(answ));
    }
    else
    {
      fprintf(stderr, "SQL error during update: %s\n", sqlite3_errmsg(db));
      strcpy(answ, "Eroare la actualizarea datelor.\n");
      write(user.fd, answ, strlen(answ));
    }

    sqlite3_finalize(statement);
  }
  else
  {
    // Record doesn't exist, perform insert
    if (sqlite3_prepare_v2(db, insertDataSQL, -1, &statement, 0) != SQLITE_OK)
    {
      fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
      strcpy(answ, "Eroare la pregatirea interogarii de inserare.\n");
      write(user.fd, answ, strlen(answ));
      return;
    }

    sqlite3_bind_text(statement, 1, user.username, -1, SQLITE_STATIC);
    sqlite3_bind_text(statement, 2, friend, -1, SQLITE_STATIC);
    sqlite3_bind_text(statement, 3, relationship, -1, SQLITE_STATIC);

    if (sqlite3_step(statement) != SQLITE_DONE)
    {
      fprintf(stderr, "SQL error during insert: %s\n", sqlite3_errmsg(db));
      strcpy(answ, "Eroare la inserarea datelor.\n");
      write(user.fd, answ, strlen(answ));
    }
    else
    {
      strcpy(answ, "Inregistrare cu succes.\n");
      write(user.fd, answ, strlen(answ));
    }

    sqlite3_finalize(statement);
  }

  pthread_mutex_unlock(&lock);

}

// NEW-POST

void new_post(struct User user)
{

  char type[21] = "";
  char column[31] = "";
  char content[10001] = "";

  strcpy(answ, "Introduceti tipul postarii (public/friend/close-friend):\n");
  write(user.fd, answ, strlen(answ));
  read(user.fd, type, 21);

  while (strcmp(type, "friend") != 0 && strcmp(type, "close-friend") != 0 && strcmp(type, "public") != 0)
  {
    strcpy(answ, "Introduceti doar cuvantul cheie (public/friend/close-friend):\n");
    write(user.fd, answ, strlen(answ));
    if (read(user.fd, type, 21) <= 0) {
      printf("Client disconnected.\n");
      pthread_exit(NULL);
    }
  }

  const char *selectDataSQL;
  const char *updateDataSQL;
  const char *insertDataSQL;

  if (type[0] == 'f')
  {
    selectDataSQL = "SELECT friend_posts FROM posts WHERE username = ?;";
    updateDataSQL = "UPDATE posts SET friend_posts = ? WHERE username = ?;";
    insertDataSQL = "INSERT INTO posts (username, friend_posts) VALUES (?, ?);";
  }
  else if (type[0] == 'c')
  {
    selectDataSQL = "SELECT close_friend_posts FROM posts WHERE username = ?;";
    updateDataSQL = "UPDATE posts SET close_friend_posts = ? WHERE username = ?;";
    insertDataSQL = "INSERT INTO posts (username, close_friend_posts) VALUES (?, ?);";
  }
  else
  {
    selectDataSQL = "SELECT public_posts FROM posts WHERE username = ?;";
    updateDataSQL = "UPDATE posts SET public_posts = ? WHERE username = ?;";
    insertDataSQL = "INSERT INTO posts (username, public_posts) VALUES (?, ?);";
  }

  strcpy(answ, "Introduceti continutul postarii ...\n");
  write(user.fd, answ, strlen(answ));
  read(user.fd, content, 10001);

  // generating the post timestamp

  time_t currentTime;
  struct tm *timeInfo;

  time(&currentTime);

  timeInfo = localtime(&currentTime);

  char formattedDateTime[20];
  strftime(formattedDateTime, sizeof(formattedDateTime), "%Y-%m-%d %H:%M", timeInfo);

  // adding the content in the data base

  sqlite3_stmt *statement;

  if (sqlite3_prepare_v2(db, selectDataSQL, -1, &statement, 0) == SQLITE_OK)
  {
    sqlite3_bind_text(statement, 1, user.username, -1, SQLITE_STATIC);

    int stepResult = sqlite3_step(statement);

    pthread_mutex_lock(&lock);

    if (stepResult != SQLITE_DONE)
    {
      const char *stored_content = (const char *)sqlite3_column_text(statement, 0);

      // formatting the post

      char post[100001];
      sprintf(post, "%s%s     %s\n%s\n\n", stored_content, user.username, formattedDateTime, content);

      if (strncmp(post, "(null)", 6) == 0 ) {
        strcpy(post, post + 6);
      }


      if (sqlite3_prepare_v2(db, updateDataSQL, -1, &statement, 0) == SQLITE_OK)
      {
        sqlite3_bind_text(statement, 1, post, -1, SQLITE_STATIC);
        sqlite3_bind_text(statement, 2, user.username, -1, SQLITE_STATIC);

        if (sqlite3_step(statement) == SQLITE_DONE)
        {
          strcpy(answ, "Modificat cu succes.\n");
          write(user.fd, answ, strlen(answ));
        }
        else
        {
          fprintf(stderr, "SQL error during update: %s\n", sqlite3_errmsg(db));
          strcpy(answ, "Eroare la actualizarea datelor.\n");
          write(user.fd, answ, strlen(answ));
        }
      }
      else
      {
        fprintf(stderr, "SQL error : %s\n", sqlite3_errmsg(db));
        strcpy(answ, "Eroare la pregatirea actualizarii.\n");
        write(user.fd, answ, strlen(answ));
      }

    }
    else
    {
      // User doesn't have posts, insert new data
      char post[10200];
      sprintf(post, "%s     %s\n%s\n\n", user.username, formattedDateTime, content);

      if (sqlite3_prepare_v2(db, insertDataSQL, -1, &statement, 0) != SQLITE_OK)
      {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        strcpy(answ, "Eroare la pregatirea interogarii de inserare.\n");
        write(user.fd, answ, strlen(answ));
        return;
      }

      sqlite3_bind_text(statement, 1, user.username, -1, SQLITE_STATIC);
      sqlite3_bind_text(statement, 2, post, -1, SQLITE_STATIC);

      if (sqlite3_step(statement) != SQLITE_DONE)
      {
        fprintf(stderr, "SQL error during insert: %s\n", sqlite3_errmsg(db));
        strcpy(answ, "Eroare la inserarea datelor.\n");
        write(user.fd, answ, strlen(answ));
      }
      else
      {
        strcpy(answ, "Inregistrare cu succes.\n");
        write(user.fd, answ, strlen(answ));
      }

      sqlite3_finalize(statement);
    }

    pthread_mutex_unlock(&lock);

  }
  else
  {
    strcpy(answ, "A intervenit o eroare.\n");
    write(user.fd, answ, strlen(answ));
  }
}

// SHOW-PROFILE

void show_profile(char username[], struct User user, int status)
{
  sqlite3_stmt *statement;
  int a = 0;

  if (status == 1 && (a = checkFriends(user.username, username)) == 1 || checkFriends(username, user.username))
  {

    const char *selectDataSQL = "SELECT type_of_friend FROM friends WHERE host_username = ? AND friend_username = ?;";

    if (sqlite3_prepare_v2(db, selectDataSQL, -1, &statement, 0) == SQLITE_OK)
    {
      if (a)
      {
        sqlite3_bind_text(statement, 1, user.username, -1, SQLITE_STATIC);
        sqlite3_bind_text(statement, 2, username, -1, SQLITE_STATIC);
      }
      else
      {
        sqlite3_bind_text(statement, 2, user.username, -1, SQLITE_STATIC);
        sqlite3_bind_text(statement, 1, username, -1, SQLITE_STATIC);
      }

      if (sqlite3_step(statement) == SQLITE_ROW)
      {

        const char *type = (const char *)sqlite3_column_text(statement, 0);

        const char *selectDataSQL;

        if (type[0] == 'f')
        {
          selectDataSQL = "SELECT friend_posts, public_posts FROM posts WHERE username = ?;";
        }
        else
        {
          selectDataSQL = "SELECT close_friend_posts, public_posts FROM posts WHERE username = ?;";
        }

        if (sqlite3_prepare_v2(db, selectDataSQL, -1, &statement, 0) == SQLITE_OK)
        {
          sqlite3_bind_text(statement, 1, username, -1, SQLITE_STATIC);

          if (sqlite3_step(statement) != SQLITE_DONE)
          {
            int ok = 0;
            const char *friend_posts = (const char *)sqlite3_column_text(statement, 0);
            const char *public_posts = (const char *)sqlite3_column_text(statement, 1);

            if (sqlite3_column_type(statement, 0) != SQLITE_NULL && strlen(friend_posts) > 0)
            {
              write(user.fd, friend_posts, strlen(friend_posts));
              ok = 1;
            }
              

            if (sqlite3_column_type(statement, 1) != SQLITE_NULL && strlen(public_posts) > 0)
            {
              write(user.fd, public_posts, strlen(public_posts));
              ok = 1;
            }
              
            if ( ok == 0 ){
              strcpy(answ, "Utilizatorul nu are postari.\n");
              write(user.fd, answ, strlen(answ));
            }
          }
          else
          {
            strcpy(answ, "Utilizatorul nu are noutati publicate.\n");
            write(user.fd, answ, strlen(answ));
          }
        }
        else
        {
          strcpy(answ, "A intervenit o eroare (1).\n");
          write(user.fd, answ, strlen(answ));
        }
      }
      else
      {
        strcpy(answ, "A intervenit o eroare (2).\n");
        write(user.fd, answ, strlen(answ));
      }
    }
    else
    {
      fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
      strcpy(answ, "A intervenit o eroare (3).\n");
      write(user.fd, answ, strlen(answ));
    }
  }
  else
  {
    const char *selectDataSQL = "SELECT public_posts from posts NATURAL JOIN users WHERE type_of_profile = 'public' AND username = ?;";

    if (sqlite3_prepare_v2(db, selectDataSQL, -1, &statement, 0) == SQLITE_OK)
    {
      sqlite3_bind_text(statement, 1, username, -1, SQLITE_STATIC);

      int selectResult = sqlite3_step(statement);

      if (selectResult != SQLITE_DONE)
      {
        const char *public_posts = (const char *)sqlite3_column_text(statement, 0);
        write(user.fd, public_posts, strlen(public_posts));
      }
      else
      {
        strcpy(answ, "Utilizatorul nu are nici o postare publica.\n");
        write(user.fd, answ, strlen(answ));
      }
    }
    else
    {
      strcpy(answ, "A intervenit o eroare (4).\n");
      write(user.fd, answ, strlen(answ));
    }
  }

}

// CHAT

void chat(int fds[], int n)
{

  char message[1001];
  bzero(message, 1001);

  for (int i = 0; i < n; i++)
  {
    strcpy(answ, "Pregatire chat...\n");
    write(fds[i], answ, strlen(answ));
    
  }

  sleep(10);

  int accepted = n;

  while (accepted > 1)
  {
    fd_set readfds;
    int nfds = fds[0];
    FD_ZERO(&readfds);
    accepted = 0;

    for (int i = 0; i < n; i++)
    {
      if ( userByFd(fds[i])->chat_accepted )
        {
          FD_SET(fds[i], &readfds);
          if (nfds < fds[i])
            nfds = fds[i];
          accepted ++;
        }
      
    }

    if (select(nfds + 1, &readfds, NULL, NULL, NULL) < 0)
    {
      perror("[server]Eroare la select().\n");
    }

    for (int i = 0; i < accepted; i++)
    {

      if (FD_ISSET(fds[i], &readfds))
      {
        bzero(message, 1001);
        if (read(fds[i], message, 1001) <= 0 || strcmp(message, "stop-chat") == 0)
        {

          strcpy(answ, "Ati iesit din chat.\n");
          write(fds[i], answ, strlen(answ));
          userByFd(fds[i])->chat_accepted = 0;
          fds[i] = fds[accepted - 1];
          accepted--;
          n--;
          if (accepted == 1)
          {
            strcpy(answ, "Chat inchis.\n");
            write(fds[0], answ, strlen(answ));
            userByFd(fds[0])->chat_accepted = 0;
            FD_ZERO(&readfds);
            break;
          }

          break;
        }
        else
        {
          for (int j = 0; j < accepted; j++)
          {
            if (fds[j] != fds[i])
            {
              char aux[1200];
              bzero(aux, 1200);
              strcpy(aux, userByFd(fds[i])->username);
              strcat(aux, " : ");
              strcat(aux, message);
              strcat(aux, "\n");
              write(fds[j], aux, strlen(aux));
            }
          }
        }
      }
    }

  }
}

static void *treat(void *);
void handleCommand(thData *td);

int main()
{

  struct sockaddr_in server;
  struct sockaddr_in from;
  int sd;
  int nr;
  int pid;
  pthread_t th[100];
  int i = 0;

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

  if (pthread_mutex_init(&lock, NULL) != 0) 
  { 
    printf("\n mutex init has failed\n"); 
    return 1; 
  } 

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("[server]Eroare la socket().\n");
    return errno;
  }

  int on = 1;
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  bzero(&server, sizeof(server));
  bzero(&from, sizeof(from));

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(PORT);

  if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[server]Eroare la bind().\n");
    return errno;
  }

  if (listen(sd, 10001) == -1)
  {
    perror("[server]Eroare la listen().\n");
    return errno;
  }

  while (1)
  {

    int client;
    thData *td;
    int length = sizeof(from);

    printf("[server]Asteptam la portul %d...\n", PORT);
    fflush(stdout);

    if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
    {
      perror("[server]Eroare la accept().\n");
      continue;
    }

    td = (struct thData *)malloc(sizeof(struct thData));
    td->idThread = i++;
    td->cl = client;

    users[client_index].fd = client;
    strcpy(users[client_index].username, "");
    users[client_index].chat_request = 0;
    users[client_index].chat_accepted = 0;
    client_index++;

    pthread_create(&th[i], NULL, &treat, td);
  }
}

static void *treat(void *arg)
{
  struct thData tdL;
  tdL = *((struct thData *)arg);
  printf("[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
  fflush(stdout);
  pthread_detach(pthread_self());
  handleCommand((struct thData *)arg);
  close((intptr_t) arg);
  return (NULL);
}

void handleCommand(thData *td)
{

  while (1)
  { 
    
    int i;
    char command[10001] = "";

    if (read(td->cl, command, sizeof(command)) <= 0 ){
      for( i = 0; i < client_index; i++ ){
        if ( td->cl == users[i].fd ) {
          users[i] = users[client_index - 1];
          client_index --;
        }
      }
      
      printf("[Thread %d] Client disconnected.\n", td->idThread);
      break;
    }
   
    if (strcmp(command, "accept") == 0)
    {

      for (i = 0; i < client_index; i++)
      {

        if ( users[i].fd == td->cl && users[i].chat_request && users[i].chat_accepted == 0)
        {
          users[i].chat_accepted = 1;
          users[i].chat_request = 0;
          while (users[i].chat_accepted)
          {
            sleep(1);
          }
          break;
        }
        else if (users[i].fd == td->cl && users[i].chat_request == 0)
        {
          strcpy(answ, "Comanda necunoscuta.\n");
          write(td->cl, answ, strlen(answ));
          break;
        }
      }
    }
    else if (strcmp(command, "login") == 0)
    {
      printf("[Thread %d]Handling login command...\n", td->idThread);

      for (i = 0; i < client_index; i++)
      {
        if (users[i].fd == td->cl)
          break;
      }
      login(&users[i]);
    }
    else if (strcmp(command, "registration") == 0)
    {
      printf("[Thread %d]Handling registration command...\n", td->idThread);

      for (i = 0; i < client_index; i++)
      {
        if (users[i].fd == td->cl)
        {
          if (strlen(users[i].username) > 0)
          {
            strcpy(answ, "Sunteti autentificat.\n");
            write(td->cl, answ, strlen(answ));
          }
          else
          {
            registration(td->cl);
          }
          break;
        }
      }
    }
    else if (strcmp(command, "logout") == 0)
    {
      printf("[Thread %d]Handling logout command...\n", td->idThread);

      for (i = 0; i < client_index; i++)
      {
        if (users[i].fd == td->cl)
        {

          if (strlen(users[i].username) > 0)
          {
            strcpy(users[i].username, "");
            strcpy(answ, "Logout cu succes.\n");
            write(td->cl, answ, strlen(answ));
          }
          else
          {
            strcpy(answ, "Nu sunteti autentificat.\n");
            write(td->cl, answ, strlen(answ));
          }
          break;
        }
      }
    }
    else if (strncmp(command, "add-friend : ", 13) == 0)
    {
      printf("[Thread %d]Handling add-friend command...\n", td->idThread);

      for (i = 0; i < client_index; i++)
      {
        if (users[i].fd == td->cl)
          break;
      }
      if (strlen(users[i].username) > 0 && userExists(command + 13) == 1)
      {
        if (strcmp(users[i].username, command + 13) == 0)
        {
          strcpy(answ, "Nu puteti sa va adaugati ca prieten propriu.\n");
          write(td->cl, answ, strlen(answ));
        }
        else
        {
          add_friend(users[i], command + 13);
        }
      }
      else
      {
        strcpy(answ, "Nu sunteti autentificati sau ati introdus un cont inexistent.\n");
        write(td->cl, answ, strlen(answ));
      }
    }
    else if (strcmp(command, "new-post") == 0)
    {
      printf("[Thread %d]Handling new-post command...\n", td->idThread);

      for (i = 0; i < client_index; i++)
      {
        if (users[i].fd == td->cl)
          break;
      }
      if(strlen(users[i].username) > 0 ){
        new_post(users[i]);
      } else {
        strcpy(answ, "Nu sunteti autentificat.\n");
        write(td->cl, answ, strlen(answ));
      }
      
    }
    else if (strcmp(command, "show-users") == 0)
    {
      printf("[Thread %d]Handling show-users command...\n", td->idThread);
      show_users(td->cl);
    }
    else if (strncmp(command, "chat : ", 7) == 0)
    {
      printf("[Thread %d]Handling chat command...\n", td->idThread);

      for (i = 0; i < client_index; i++)
      {
        if (users[i].fd == td->cl)
        {
          if (strlen(users[i].username) > 0)
          {
            char *token = strtok(command + 7, " ");
            int k = 1;
            int *fds = malloc(999*(sizeof(int)));

            if (fds == NULL)
            {
              perror("Failed to allocate memory for a_users");
              return;
            }

            fds[0] = users[i].fd;
            users[i].chat_accepted = 1;

            while (token != NULL)
            {

              for (int q = 0; q < client_index; q++)
              {
                if (strcmp(users[q].username, token) == 0 && q != i)
                {
                  fds[k] = users[q].fd;
                  users[q].chat_request = 1;
                  k++;
                }
              }
              token = strtok(NULL, " ");
            }

            if (k - 1 > 0)
            {
                chat(fds, k);
            }
            else
            {
              strcpy(answ, "Din pacate utilizatorii mentionati nu sunt activi.\n");
              write(td->cl, answ, strlen(answ));
            }

            // Free the dynamically allocated memory
            free(fds);
          } else {
              strcpy(answ, "Nu sunteti autentificat.\n");
              write(td->cl, answ, strlen(answ));
          }
        }
      }
    }
    else if (strncmp(command, "show-profile : ", 15) == 0)
    {
      printf("[Thread %d]Handling show-profile command...\n", td->idThread);
      for (i = 0; i < client_index; i++)
      {
        if (users[i].fd == td->cl)
          break;
      }
      if (strlen(users[i].username) > 0 && userExists(command + 15) == 1)
      {
        show_profile(command + 15, users[i], 1);
      }
      else if (userExists(command + 15) == 1)
      {
        show_profile(command + 15, users[i], 0);
      }
      else
      {
        strcpy(answ, "Ati introdus un cont inexistent, incercati show-users.\n");
        write(td->cl, answ, strlen(answ));
      }
    }
    else
    {
      printf("[Thread %d]Unknown command: %s\n", td->idThread, command);

      strcpy(answ, "Comanda necunoscuta.\n");
      write(td->cl, answ, strlen(answ));
    }
  }
  close(td->cl);
  free(td);
  pthread_exit(NULL);
}
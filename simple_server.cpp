/* run using ./server <port> */
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>

#include <pthread.h>

#include "http_server.hh"

#define THREAD_COUNT 40
#define MAXCLIENT 10000

char buffer[2048];
int n;

pthread_t threadpool[THREAD_COUNT];
int flag;

pthread_cond_t Queuecond;
pthread_cond_t Queuefull;
pthread_mutex_t Queuemutex;
int Clients_total;
int Clients_buffer[MAXCLIENT];

void error(char *msg)
{
  perror(msg);
  exit(1);
}

// actual task for thread.
void *clientaction(void *argv)
{
  // int newsockfd = *((int *)newsockfd);
  // bzero(buffer, 2048);
  int client, i;
  while (1)
  {
    pthread_mutex_lock(&Queuemutex);
    while (Clients_total == 0)
    {
      pthread_cond_wait(&Queuecond, &Queuemutex);

      if (flag == 1)
      {
        pthread_mutex_unlock(&Queuemutex);
        pthread_exit(NULL);
      }
    }

    client = Clients_buffer[0];

    for (i = 0; i < (Clients_total - 1); i++)
    {
      Clients_buffer[i] = Clients_buffer[i + 1];
    }

    Clients_total--;
    pthread_cond_signal(&Queuefull);
    pthread_mutex_unlock(&Queuemutex);

    // cout<<"print1\n";
    bzero(buffer, 2048);
    n = read(client, buffer, 2047);
    if (n == 0)
      continue;
    if (n < 0)
      error("ERROR reading from socket");
    // printf("Here is the message: %s", buffer);
    // cout<<"print2\n";
    HTTP_Response *res = handle_request(buffer);
    // cout<<"print2.5\n";
    string res1 = res->get_string();
    // cout<<"print3\n";
    /* send reply to client */

    n = write(client, res1.c_str(), res1.size());
    // n = write(newsockfd, buffer, strlen(buffer));
    if (n < 0)
      error("ERROR writing to socket");
    // bzero(buffer, 2048);

    // cout<<"print4\n";
    close(client);
    delete (res);
  } // return NULL;
}

// action on threads after creation.

// void *thread_waiting(void *t)
// {
//   while (1)
//   {

//     pthread_mutex_lock(&Queuemutex);
//     while (Clients_total == 0)
//     {
//       pthread_cond_wait(&Queuecond, &Queuemutex);
//     }

//     int client = Clients_buffer[0], i;

//     for (i = 0; i < (Clients_total - 1); i++)
//     {
//       Clients_buffer[i] = Clients_buffer[i + 1];
//     }

//     Clients_total--;
//     pthread_cond_signal(&Queuefull);
//     pthread_mutex_unlock(&Queuemutex);
// void signal_thread(int newsockfd)
// {
//   // critical section for queue.
//   // update new socket fd to queue and wakeup one thread.
//   pthread_mutex_lock(&Queuemutex);
//   Clients_buffer[Clients_total++] = newsockfd;
//    pthread_cond_signal(&Queuecond);
//   pthread_mutex_unlock(&Queuemutex);

//   return;
// }
//     clientaction(client);
//   }
// }void signal_thread(int newsockfd)
// {
//   // critical section for queue.
//   // update new socket fd to queue and wakeup one thread.
//   pthread_mutex_lock(&Queuemutex);void signal_thread(int newsockfd)
// {
//   // critical section for queue.
//   // update new socket fd to queue and wakeup one thread.
//   pthread_mutex_lock(&Queuemutex);
//   Clients_buffer[Clients_total++] = newsockfd;
//    pthread_cond_signal(&Queuecond);
//   pthread_mutex_unlock(&Queuemutex);

//   return;
// }
//   Clients_buffer[Clients_total++] = newsockfd;
//    pthread_cond_signal(&Queuecond);
//   pthread_mutex_unlock(&Queuemutex);

//   return;
// }

// void signal_thread(int newsockfd)
// {
//   // critical section for queue.
//   // update new socket fd to queue and wakeup one thread.
//   pthread_mutex_lock(&Queuemutex);
//   Clients_buffer[Clients_total++] = newsockfd;
//    pthread_cond_signal(&Queuecond);
//   pthread_mutex_unlock(&Queuemutex);

//   return;
// }

void func(int s)
{
  flag = 1;
  pthread_cond_broadcast(&Queuecond);
  for (int i = 0; i < THREAD_COUNT; i++)
  {
    pthread_cancel(threadpool[i]);
    pthread_join(threadpool[i], NULL);
  }
  exit(0);
}

#include <signal.h>
int main(int argc, char *argv[])
{
  signal(SIGINT, func);
  pthread_mutex_init(&Queuemutex, NULL);
  pthread_cond_init(&Queuecond, NULL);

  int sockfd, newsockfd, portno;
  int threadstatus;
  socklen_t clilen;
  // char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;
  // int n;

  if (argc < 2)
  {
    fprintf(stderr, "ERROR, no port provided\n");
    exit(1);
  }

  /* create socket */

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  /* fill in port number to listen on. IP address can be anything (INADDR_ANY)
   */

  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  /* bind socket to this port number on this machine */

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR on binding");

  /* listen for incoming connection requests */

  listen(sockfd, 10000);
  clilen = sizeof(cli_addr);

  // threadpool creation.

  // pthread_t threadpool[THREAD_COUNT];

  for (int i = 0; i < THREAD_COUNT; i++)
  {
    threadstatus = pthread_create(&threadpool[i], NULL, clientaction, NULL);
  }
  if (threadstatus)
  {
    cout << "Error:unable to create thread," << threadstatus << endl;
    exit(-1);
  }

  /* accept a new request, create a newsockfd */

  while (1)
  {
    pthread_mutex_lock(&Queuemutex);
    if (Clients_total == MAXCLIENT)
    {
      pthread_cond_wait(&Queuefull, &Queuemutex);
    }
    pthread_mutex_unlock(&Queuemutex);
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    pthread_mutex_lock(&Queuemutex);
    Clients_buffer[Clients_total++] = newsockfd;
    pthread_cond_signal(&Queuecond);
    pthread_mutex_unlock(&Queuemutex);

    // signal_thread(newsockfd);
    /*if (newsockfd < 0)
      error("ERROR on accept");
    else
    {
      /* read message from client */

    // signal_thread(newsockfd);

    // pthread_t thread_id;
    // pthread_create(&thread_id, NULL, clientaction, &newsockfd);
    //}
    // clientaction(newsockfd);*/
  }
  return 0;
}
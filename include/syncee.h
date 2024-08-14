#include <pthread.h>
#include <sys/socket.h>

#ifndef SYNCEE_H
#define SYNCEE_H

typedef struct syncee_thread_list {
  pthread_t thread;
  struct thread_list *next;
} SYNCEE_THREAD_LIST;

typedef struct syncee_args {
  int ipc_client;
  int port;
  int ip_type;
  char *server_addr;
  char *requested_file;
} SYNCEE_ARGS;

void *syncee_init(SYNCEE_ARGS *args);

#endif

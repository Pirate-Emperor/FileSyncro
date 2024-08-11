#include <pthread.h>
#include <sys/types.h>

#ifndef SYNCER_H
#define SYNCER_H
typedef struct syncer_args {
  int port;
  char *dir;
  int *close_server;
  pthread_mutex_t *close_server_mutex;
} SYNCER_ARGS;

void *syncer_init(SYNCER_ARGS *args);
#endif

#include "syncer.h"
#include "file-table.h"
#include "message-definition.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_PENDING_CONECTIONS 10
#define MAX_RECEIVER_BUFFER_SIZE 128
#define MAX_SENDING_BUFFER_SIZE 1024
#define ACCEPT_LOOP_TIMEOUT 100

typedef int SOCKET;
typedef struct syncer_receiver_args {
  SOCKET socket;
  FILE_TABLE *files;
  char *dir;
  pthread_mutex_t *files_mutex;
} SYNCER_RECEIVER_ARGS;

typedef struct thread_list {
  pthread_t thread;
  struct thread_list *next;
} THREAD_LIST;

void *syncer_receiver(SYNCER_RECEIVER_ARGS *args);
void send_file_table(SOCKET client, FILE_TABLE *files);
void send_file_content(SOCKET client, FILE_TABLE *files, char *dir, char *file);

void *syncer_init(SYNCER_ARGS *args) {
  SOCKET listener;
  struct sockaddr_in addr;
  FILE_TABLE *files;
  pthread_mutex_t files_mutex;
  THREAD_LIST *client_threads;
  struct pollfd poll_fds;
  printf("Syncer thread created.\n");

  printf("Creating Listener Socket.\n");
  // Create Listener Socket and Bind it to Port
  listener = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (listener < 0) {
    fprintf(stderr, "Wasn't able to create listener socket.\n");
    free(args);
    pthread_exit(NULL);
  }

  addr.sin_family = AF_INET;
  addr.sin_port = htons(args->port);
  addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    fprintf(stderr, "Wasn't able to bind listener socket to port %d.\n",
            args->port);
    free(args);
    pthread_exit(NULL);
  }

  if (listen(listener, MAX_PENDING_CONECTIONS) != 0) {
    fprintf(stderr,
            "Wasn't able to listen for connection on listener socket.\n");
    free(args);
    pthread_exit(NULL);
  }

  printf("Scaning for files in dir '%s'.\n", args->dir);
  // Scan for Files in Folder, generate Files Table, and create its mutex
  if (get_files_in_dir(args->dir, &files) != 0) {
    fprintf(stderr, "Wasn't able to generate files table for %s.\n", args->dir);
    free(args);
    pthread_exit(NULL);
  }
  pthread_mutex_init(&files_mutex, NULL);
  // Initialize the client_threads list
  client_threads = NULL;
  // Create the Pool for accepts

  poll_fds.fd = listener;
  poll_fds.events = POLLIN;
  // Wait for new conections
  printf("Waiting for new conections.\n");
  int _close_server = 0;
  int *close_server = args->close_server;
  while (_close_server == 0) {
    SOCKET client;
    struct sockaddr client_addr;
    socklen_t client_addr_len;
    // Wait for new connections
    int rc = poll(&poll_fds, 1, ACCEPT_LOOP_TIMEOUT);
    if (rc < 0) {
      char *error = malloc(sizeof(char) * 50);
      strerror_r(errno, error, 50);
      fprintf(stderr, "Pooling failed: %s.\n", error);
      free(error);
      break;
    }
    if (rc == 0) { // Just a timeout
      pthread_mutex_lock(args->close_server_mutex);
      _close_server = *close_server;
      pthread_mutex_unlock(args->close_server_mutex);
      continue;
    }
    client = accept(listener, &client_addr, &client_addr_len);
    if (client < 0) {
      if (errno != EWOULDBLOCK || errno != EAGAIN) {
        char *error = malloc(sizeof(char) * 50);
        strerror_r(errno, error, 50);
        fprintf(stderr, "Accepting conection of client failed: %s.\n", error);
        free(error);
      }
      continue;
    }
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &((struct sockaddr_in *)&client_addr)->sin_addr,
              client_ip, INET_ADDRSTRLEN);
    printf("New conection from %s.\n", client_ip);
    // Give new socket to client thread
    pthread_t client_thread;
    SYNCER_RECEIVER_ARGS *receiver_args =
        calloc(1, sizeof(SYNCER_RECEIVER_ARGS));
    receiver_args->files = files;
    receiver_args->dir = args->dir;
    receiver_args->files_mutex = &files_mutex;
    receiver_args->socket = client;
    pthread_create(&client_thread, NULL, (void *)syncer_receiver,
                   receiver_args);
    // Add new thread to list
    THREAD_LIST *new = calloc(1, sizeof(THREAD_LIST));
    new->thread = client_thread;
    new->next = client_threads;
    client_threads = new;
  }
  printf("Closing Listener Socket.\n");
  // Close listening socket
  if (close(listener) != 0) {
    fprintf(stderr, "Failed to close listener socket.\n");
  }
  // Wait for client threads to finish, and free threads list
  THREAD_LIST *next_client_thread;
  while (client_threads != NULL) {
    next_client_thread = client_threads->next;
    pthread_join(client_threads->thread, NULL);
    free(client_threads);
    client_threads = next_client_thread;
  }
  // Free and Exit
  free(args);
  FILE_TABLE *next;
  while (files != NULL) {
    next = files->next;
    free(files->file);
    free(files);
    files = next;
  }
  pthread_mutex_destroy(&files_mutex);
  pthread_exit(NULL);
}

void *syncer_receiver(SYNCER_RECEIVER_ARGS *args) {
  SOCKET client = args->socket;
  FILE_TABLE *file_table = args->files;
  char *dir = args->dir;
  pthread_mutex_t *files_mutex = args->files_mutex;

  printf("New syncer receiver thread.\n");

  char message_header;
  int received_amount;

  // Read Message Header
  received_amount = recv(client, &message_header, sizeof(message_header), 0);
  if (received_amount < sizeof(message_header)) {
    fprintf(stderr, "Error receiving Header from client.\n");
    goto CLOSE_AND_FREE;
  }
  // Filter message type
  if (message_header == FILE_TABLE_REQUEST) { // TABLE REQUEST
    // Send file table
    // Locking it as file_table is shared, even if in "readonly" mode
    pthread_mutex_lock(files_mutex);
    send_file_table(client, file_table);
    pthread_mutex_unlock(files_mutex);
  } else if (message_header == FILE_CONTENT_REQUEST) { // FILE CONTENT REQUEST
    // Receive string
    char *file_name;
    int status = read_string_from_socket(client, &file_name);
    if (status == -1) {
      fprintf(stderr, "Error receiving string size from client.\n");
      goto CLOSE_AND_FREE;

    } else if (status == -2) {
      fprintf(stderr, "Error receiving string from client.\n");
      goto CLOSE_AND_FREE;
    }
    // Send file contents
    send_file_content(client, file_table, dir, file_name);
  }

CLOSE_AND_FREE:
  // Close client socket
  printf("Closing conection to client.\n");
  if (close(client) != 0) {
    fprintf(stderr, "Failed to close client socket.\n");
  }
  // Free and exit
  free(args); // We dont need to release the file_table, as it is from
              // syncer_init, not ours
  pthread_exit(NULL);
}

// File Table Request Message Format:
// HEADER (FILE_TABLE_REQUEST)
// [ STRING_SEPARATOR STRING_SIZE STRING ]*
void send_file_table(SOCKET client, FILE_TABLE *files) {
  char header = FILE_TABLE_REQUEST;
  char string_separator = FILE_TABLE_STRING_SEPARATOR;
  FILE_TABLE *pointer;

  printf("Sending file table to client.\n");

  // Send Header
  send(client, &header, sizeof(header), MSG_MORE);
  // Send File Names
  pointer = files;
  while (pointer != NULL) {
    send(client, &string_separator, sizeof(char), MSG_MORE);
    write_string_to_socket(pointer->file, client);
    pointer = pointer->next;
  }
}
// File Content Message Format:
// HEADER (FILE_CONTENT_REQUEST)
// FILE CONTENT
void send_file_content(SOCKET client, FILE_TABLE *files, char *dir,
                       char *file) {
  char buffer[MAX_SENDING_BUFFER_SIZE];
  char header = FILE_CONTENT_REQUEST;
  FILE *file_handler;

  int file_in_table = 0;
  FILE_TABLE *pointer;
  pointer = files;
  while (pointer != NULL && file_in_table == 0) {
    if (strcmp(pointer->file, file) == 0)
      file_in_table = 1;
    pointer = pointer->next;
  }
  if (file_in_table == 1)
    printf("Sending contents of file '%s' to client.\n", file);
  else
    printf("File '%s' doesnt exist in table, sending blank.\n", file);

  // Send Header
  send(client, &header, sizeof(header), MSG_MORE);
  // Send File
  if (file_in_table) {
    char *file_path = calloc(strlen(dir) + 1 + strlen(file) + 1, sizeof(char));
    strcat(file_path, dir);
    strcat(file_path, "/");
    strcat(file_path, file);
    printf("Opening file %s.\n", file_path);
    file_handler = fopen(file_path, "r");
    if (file_handler != NULL) {
      size_t read_bytes;
      while ((read_bytes = fread(buffer, sizeof(char), MAX_SENDING_BUFFER_SIZE,
                                 file_handler)) > 0) {
        int send_status = send(client, buffer, sizeof(char) * read_bytes, 0);
        if (send_status < 0) {
          fprintf(stderr,
                  "Sending file '%s' to client had the following error: %s\n",
                  file, strerror(errno));
          fclose(file_handler);
          return;
        }
      }
      fclose(file_handler);
    } else {
      fprintf(stderr,
              "Failed to open file %s, returning empty bytes to client.\n",
              file);
    }
  }
}

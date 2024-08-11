#include "syncee.h"
#include "message-definition.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define RECEIVE_FILE_BUFFER_SIZE 1024

typedef int SOCKET;

int get_file_table_for_ipc(SOCKET server, SOCKET ipc_client);
int get_file_contents_for_ipc(SOCKET server, char *file, SOCKET ipc_client);

void *syncee_init(SYNCEE_ARGS *args) {
  SOCKET server;
  struct sockaddr_in server_addr_in;
  SOCKET ipc_client = args->ipc_client;
  printf("Syncee thread created.\n");

  // Create Client Socket and Connect it to Server
  server = socket(AF_INET, SOCK_STREAM, 0);
  if (server < 0) {
    fprintf(stderr, "Wasn't able to create client socket.\n");
    goto CLOSE_FREE_AND_EXIT;
  }
  server_addr_in.sin_family = args->ip_type;
  server_addr_in.sin_port = htons(args->port);
  // Convert IP ADDR to number
  if (inet_pton(args->ip_type, args->server_addr, &server_addr_in.sin_addr) <=
      0) {
    fprintf(stderr, "Invalid address %s\n", args->server_addr);
    goto CLOSE_FREE_AND_EXIT;
  }
  // Conect to server
  printf("Conecting to server in %s.\n", args->server_addr);
  if (connect(server, (struct sockaddr *)&server_addr_in,
              sizeof(server_addr_in)) < 0) {
    fprintf(stderr, "Wasn't able to connect to server in %s.\n",
            args->server_addr);
    goto CLOSE_FREE_AND_EXIT;
  }

  // Process IPC request
  if (args->requested_file == NULL) { // IPC request for file table
    get_file_table_for_ipc(server, ipc_client);
  } else { // IPC Request For File Data from server
    printf("Asking server in %s for file data of '%s'.\n", args->server_addr,
           args->requested_file);
    get_file_contents_for_ipc(server, args->requested_file, ipc_client);
  }

CLOSE_FREE_AND_EXIT:
  printf("Finishing connection to %s.\n", args->server_addr);
  // Close Sockets
  if (close(server) != 0) {
    fprintf(stderr, "Failed to close client socket.\n");
  }
  close(ipc_client);
  free(args->server_addr);
  free(args->requested_file);
  free(args);
  return 0;
}

// Ask For File Table from server
// printf("Asking server in %s for file table.\n", args->server_addr);
int get_file_table_for_ipc(SOCKET server, SOCKET ipc_client) {
  char message_header;
  int received_amount;

  // Send Request For File Table from server
  char file_table_request_header = FILE_TABLE_REQUEST;
  send(server, &file_table_request_header, sizeof(char), 0);
  // Receive File Table
  received_amount = recv(server, &message_header, sizeof(message_header), 0);
  if (received_amount < sizeof(message_header)) {
    fprintf(stderr, "Error receiving Header from server.\n");
    return -1;
  }
  // Check if the Header is correct
  if (message_header != FILE_TABLE_REQUEST) {
    fprintf(stderr, "Server didnt send File Table Header.\n");
    return -1;
  }
  // Read and create file table
  printf("Receiving File Table from server.\n");
  int received_it_all = 0;
  char separator_read;
  char *file_name_string;
  while (received_it_all == 0) {
    received_amount = recv(server, &separator_read, sizeof(char), 0);
    if (received_amount < sizeof(char)) {
      fprintf(stderr, "Error receiving separator from server.\n");
      break;
    }
    if (separator_read == FILE_TABLE_STRING_SEPARATOR) {
      int status = read_string_from_socket(server, &file_name_string);
      if (status == -1) {
        fprintf(stderr, "Error receiving string size from server.\n");
        break;
      } else if (status == -2) {
        fprintf(stderr, "Error receiving string from server.\n");
        break;
      }

      // Send request back to IPC client
      int name_size = strlen(file_name_string);
      char *formated_output = calloc(name_size + 2, sizeof(char));
      strcpy(formated_output, file_name_string);
      formated_output[name_size] = '\n';
      send(ipc_client, formated_output, sizeof(char) * strlen(formated_output),
           0);
      printf("FILE: %s.\n", file_name_string);
      free(formated_output);
      free(file_name_string);

    } else if (separator_read == FILE_TABLE_MESSAGE_END) {
      printf("Received end of file table.\n");
      received_it_all = 1;
    } else {
      received_it_all = -1;
      fprintf(stderr, "Received wrong separator.\n");
    }
  }
  return 0;
}
int get_file_contents_for_ipc(SOCKET server, char *file, SOCKET ipc_client) {
  char message_header;
  int received_amount;

  // Send Request For File Data from server
  char table_content_request_header = FILE_CONTENT_REQUEST;
  send(server, &table_content_request_header, sizeof(char), MSG_MORE);
  write_string_to_socket(file, server);
  send(server, NULL, 0, 0); // Confirm to send the buffer

  // Receive data From server
  received_amount = recv(server, &message_header, sizeof(message_header), 0);
  if (received_amount < sizeof(message_header)) {
    fprintf(stderr, "Error receiving Header from server.\n");
    return -1;
  }
  // Check if the Header is correct
  if (message_header != FILE_CONTENT_REQUEST) {
    fprintf(stderr, "Server didnt send File Content Header.\n");
    return -1;
  }

  int received_it_all = 0;
  printf("Receiving data from server.\n");
  while (received_it_all == 0) {
    char buffer[RECEIVE_FILE_BUFFER_SIZE];
    int buffer_ocupied;
    buffer_ocupied =
        recv(server, buffer, sizeof(char) * RECEIVE_FILE_BUFFER_SIZE, 0);
    if (buffer_ocupied == -1) {
      fprintf(stderr, "Issue while receiving file\n");
      return -1;
    } else if (buffer_ocupied > 0) {
      send(ipc_client, buffer, sizeof(char) * buffer_ocupied, 0);
    } else {
      received_it_all = 1;
      printf("Received end of file\n");
    }
  }
  return 0;
}

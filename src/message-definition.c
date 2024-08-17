#include "message-definition.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

typedef int SOCKET;

int write_string_to_socket(char *string, SOCKET socket) {
  int32_t string_lenght = htonl(strlen(string));
  send(socket, &string_lenght, sizeof(string_lenght), MSG_MORE);
  send(socket, string, sizeof(char) * strlen(string), MSG_MORE);
}

int read_string_from_socket(SOCKET socket, char **string) {
  int received_amount;
  int32_t received_string_lenght;
  int string_lenght;
  char *_string;

  received_amount = recv(socket, &received_string_lenght, sizeof(int32_t), 0);
  if (received_amount < sizeof(int32_t)) {
    return -1;
  }
  string_lenght = ntohl(received_string_lenght);
  _string = calloc(string_lenght + 1, sizeof(char));

  int string_left = string_lenght;
  while (string_left > 0) {
    char *end_of_string = _string + (string_lenght - string_left);
    received_amount =
        recv(socket, end_of_string, string_left * sizeof(char), 0);
    if (received_amount <= 0) {
      free(_string);
      return -2;
    }
    string_left -= received_amount;
  }
  *string = _string;
  return 0;
}

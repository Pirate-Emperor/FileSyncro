#include "ipc-connection-definition.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef int SOCKET;
int read_ipc_socket_string(SOCKET ipc_client, char **string) {
  char buffer[20];
  int reached_the_end = 0;
  int r, received_chars;
  char *tmp = NULL;
  char *_string;
  while (reached_the_end == 0) {
    r = recv(ipc_client, &buffer, sizeof(buffer) - 1, MSG_PEEK);
    if (r == -1) {
      if (_string != NULL)
        free(_string);
      if (tmp != NULL)
        free(tmp);
      return -1;
    }
    // we preprend to the string what was from the last iteration, if anything,
    // and read the buffer contents, untill the '\0' read. If there is no '\0',
    // then read the whole contents. A way to know which case is this is by
    // making buffer a null terminated string, and then its strlen will tell us
    // if we read the '\0' from peek (when strlen < sizeof(buffer) - 1), or if
    // there is more on the stream to read (strlen == sizeof(buffer) - 1).
    buffer[sizeof(buffer) - 1] = '\0';
    received_chars = strlen(buffer);
    // If this buffer has a \0 that isnt the one added to the end of it, this
    // means that we found the end of the string being sent
    if (received_chars < (sizeof(buffer) - 1)) {
      received_chars++;    // so we need to read the contents + the \0
      reached_the_end = 1; // and mark that we reached the end
    }
    int string_size = received_chars;
    if (tmp != NULL)
      string_size += strlen(tmp);
    _string = calloc(string_size + 1, sizeof(char));
    if (tmp != NULL)
      strcpy(_string, tmp);
    r = read(ipc_client, &_string[string_size - received_chars],
             sizeof(char) * received_chars);

    if (reached_the_end == 0) {
      if (tmp != NULL)
        free(tmp);
      tmp = _string; // current string is the prefix from the next iteration
    }
  }

  *string = _string;
  if (tmp != NULL)
    free(tmp);

  return 0;
}

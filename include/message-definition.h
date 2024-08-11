#ifndef MESSAGE_DEFINITION_H
#define MESSAGE_DEFINITION_H

#define FILE_TABLE_REQUEST 'T'
#define FILE_TABLE_STRING_SEPARATOR '+'
#define FILE_TABLE_MESSAGE_END '!'

#define FILE_CONTENT_REQUEST 'C'
#define FILE_CONTENT_MESSAGE_END EOF

int write_string_to_socket(char *string, int socket);
int read_string_from_socket(int socket, char **string);

#endif

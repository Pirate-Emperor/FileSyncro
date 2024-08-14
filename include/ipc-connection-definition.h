#ifndef IPC_CONNECTION_DEFINITION_H
#define IPC_CONNECTION_DEFINITION_H

#define SOCKET_NAME "/tmp/file-syncer.socket"

int read_ipc_socket_string(int ipc_client, char **string);

#endif

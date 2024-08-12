all: 
	make server
	make client
server:
	gcc src/file-table.c -I include/ -o out/file-table.o -c
	gcc src/message-definition.c -I include/ -o out/message-definition.o -c
	gcc src/ipc-connection-definition.c -I include/ -o out/ipc-connection-definition.o -c
	gcc src/syncee.c -lpthread -I include/ -o out/syncee.o -c
	gcc src/syncer.c -lpthread -I include/ -o out/syncer.o -c
	gcc src/main.c -lpthread out/file-table.o out/message-definition.o out/ipc-connection-definition.o out/syncee.o out/syncer.o -I include/ -o out/daemon
client:
	gcc src/message-definition.c -I include/ -o out/message-definition.o -c
	gcc src/ipc-connection-definition.c -I include/ -o out/ipc-connection-definition.o -c
	gcc src/client-main.c out/message-definition.o out/ipc-connection-definition.o -I include/ -o out/client
clean:
	rm -rf out/*

# FileSyncro

## Overview

**FileSyncro** is a file transfer service designed to facilitate on-demand file transfers between multiple machines. It utilizes a peer-to-peer (P2P) model over TCP for communication, while user requests are handled via Unix Domain Sockets, functioning like a daemon service.

---

## About the Program

FileSyncro's source code generates two executables: one for file transfers and user requests on a machine (the **daemon**), and another for user interaction with the daemon via IPC (the **client**).

## Compilation Instructions

To compile the project, ensure you have the `make` tool installed. Run the following command in the root directory of the repository:

```bash
make
```

Upon successful compilation, the binaries `daemon` and `client` will be located in the `out` directory.

---

## Running the Binaries

### Daemon

The **daemon** is the main executable that provides file transfer services to users on the same or other machines. It runs in the background, and interactions with it are performed through the **client** program.

To start the daemon, use the following command:

```bash
./daemon <directory> <port>
```

**Example:**

```bash
./daemon . 8080
```

- **`<directory>`**: The directory from which the daemon will read files and facilitate transfers. This path should be relative to the execution location of the binary, and the daemon does not recursively read directories within the specified directory.
- **`<port>`**: The port on which the server will listen for connections from other daemons.

### Client

The **client** is used to make file transfer requests to your machine or another machine on the network. Commands passed to the client are forwarded to the daemon, which returns the output to the client.

#### Available Commands:

```bash
./client close                    # Terminates the daemon process
./client list <ip> <port>         # Lists files available for transfer
./client cat <file> <ip> <port>   # Transfers file contents to standard output
```

**Examples:**

```bash
./client close
./client list 127.0.0.1 8080
./client cat readme.md 127.0.0.1 8080
```

- **`./client close`**: Requests the daemon process running on the user's machine to terminate.
- **`./client list`**: Lists the files available for transfer from the IP at `<ip>` on the specified `<port>`. `<ip>` must be provided in IPv4 format.
- **`./client cat`**: Transfers the contents of the specified `<file>` from the IP at `<ip>` on the specified `<port>` to standard output.

> **Note:** If too many arguments are passed to the client, it may hang while waiting for the server to read those arguments, even if the server has already closed the connection.

---

## Troubleshooting

### Connection Issues

If you are unable to connect despite being on the same local network, check whether the TCP ports you are using are open and allowed through your machine's firewall.

### Daemon Port Binding Issues

If the daemon fails to bind to the requested port, it may be due to improper closure of the daemon. Wait a few minutes or try a different port.

---

## Known Issues

- If the client fails to forward data in a pipe, connection errors may cause the daemon to crash.

---

## Contributing

Feel free to fork the repository, make changes, and submit pull requests. Contributions are welcome!

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Author

**Pirate-Emperor**

[![Twitter](https://skillicons.dev/icons?i=twitter)](https://twitter.com/PirateKingRahul)
[![Discord](https://skillicons.dev/icons?i=discord)](https://discord.com/users/1200728704981143634)
[![LinkedIn](https://skillicons.dev/icons?i=linkedin)](https://www.linkedin.com/in/piratekingrahul)

[![Reddit](https://img.shields.io/badge/Reddit-FF5700?style=for-the-badge&logo=reddit&logoColor=white)](https://www.reddit.com/u/PirateKingRahul)
[![Medium](https://img.shields.io/badge/Medium-42404E?style=for-the-badge&logo=medium&logoColor=white)](https://medium.com/@piratekingrahul)

- GitHub: [Pirate-Emperor](https://github.com/Pirate-Emperor)
- Reddit: [PirateKingRahul](https://www.reddit.com/u/PirateKingRahul/)
- Twitter: [PirateKingRahul](https://twitter.com/PirateKingRahul)
- Discord: [PirateKingRahul](https://discord.com/users/1200728704981143634)
- LinkedIn: [PirateKingRahul](https://www.linkedin.com/in/piratekingrahul)
- Skype: [Join Skype](https://join.skype.com/invite/yfjOJG3wv9Ki)
- Medium: [PirateKingRahul](https://medium.com/@piratekingrahul)

---

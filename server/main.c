#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "server.h"

// TODO Ajouter les attributs du client (HP...) et gérer leur perte par le serveur
// TODO Gérer l'unicité des pseudos

static void run() {
    SOCKET sock = init_connection();
    char buffer[BUF_SIZE];
    int actual = 0;                 // Array index
    int max = sock;
    Client clients[MAX_CLIENTS];    // An array for all clients

    fd_set rdfs;

    while (1) {
        int i = 0;

        // SELECT Params
        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);
        for (i = 0; i < actual; i++)
            FD_SET(clients[i].sock, &rdfs); // Adding each client's socket

        if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1) {
            perror("select()");
            exit(errno);
        }


        if (FD_ISSET(STDIN_FILENO, &rdfs))  // If something happens on standard input
            break; // Stop process when typing on keyboard
        else if (FD_ISSET(sock, &rdfs)) {   // New connection
            SOCKADDR_IN csin = {0};
            size_t sinsize = sizeof(csin);
            int csock = accept(sock, (SOCKADDR *) &csin, (socklen_t *restrict) &sinsize);
            if (csock == SOCKET_ERROR) {
                perror("accept()");
                continue;
            }

            if (read_client(csock, buffer) == -1)   // After connecting, the client sends its name
                continue; // Disconnects

            max = csock > max ? csock : max;    // returns the new max fd

            FD_SET(csock, &rdfs);

            Client c/* = {csock}*/;
            c.sock = csock;
            strncpy(c.name, buffer, BUF_SIZE - 1);
            clients[actual] = c;
            actual++;
        } else {
            int i = 0;
            for (i = 0; i < actual; i++) {
                if (FD_ISSET(clients[i].sock, &rdfs)) { // A client is talking
                    Client client = clients[i];
                    int c = read_client(clients[i].sock, buffer);
                    if (c == 0) {                       // A client disconnected
                        closesocket(clients[i].sock);
                        remove_client(clients, i, &actual);
                        strncpy(buffer, client.name, BUF_SIZE - 1);
                        strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                        send_message_to_all_clients(clients, client, actual, buffer, 1);
                    } else if (str_equals(buffer, "send")) {
                        send_message_to_client(clients, clients[i], clients[i], actual, "Destination ? ", 1);
                        int c2 = read_client(clients[i].sock, buffer);
                        if (c2) {
                            Client dest;
                            int j;
                            for (j = 0; j < actual; j++) {
                                if (str_equals(clients[j].name, buffer)) {
                                    dest = clients[j];
                                    break;
                                }
                                else
                                    strncpy(dest.name, "NULL", BUF_SIZE - 1);
                            }
                            if (!str_equals(dest.name, "NULL")) {
                                send_message_to_client(clients, clients[i], clients[i], actual, "Message ? ", 1);
                                int c3 = read_client(clients[i].sock, buffer);
                                if (c3)
                                    send_message_to_client(clients, clients[i], dest, actual, buffer, 0);
                            } else
                                send_message_to_client(clients, clients[i], clients[i], actual,
                                                       "No client found with this pseudo.", 1);
                        }
                    } else
                        send_message_to_all_clients(clients, client, actual, buffer, 0);
                    break;
                }
            }
        }
    }

    clear_clients(clients, actual);
    end_connection(sock);
}

static void clear_clients(Client *clients, int actual) {
    int i = 0;
    for (i = 0; i < actual; i++) {
        closesocket(clients[i].sock);
    }
}

static void remove_client(Client *clients, int to_remove, int *actual) {
    memmove(clients + to_remove, clients + to_remove + 1,
            (*actual - to_remove - 1) * sizeof(Client));  // We remove the client from the array
    (*actual)--;    // Reducing the number of clients
}

static void
send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server) {
    int i = 0;
    char message[BUF_SIZE];
    message[0] = 0;
    for (i = 0; i < actual; i++) {
        if (sender.sock != clients[i].sock) {
            if (from_server == 0) {
                strncpy(message, sender.name, BUF_SIZE - 1);
                strncat(message, " : ", sizeof message - strlen(message) - 1);
            }
            strncat(message, buffer, sizeof message - strlen(message) - 1);
            write_client(clients[i].sock, message);
        }
    }
}

static void
send_message_to_client(Client clients[MAX_CLIENTS], Client sender, Client receiver, int actual, const char *buffer,
                       char from_server) {
    int i = 0;
    char message[BUF_SIZE] = {0};
    for (i = 0; i < actual; i++) {
        if (str_equals(receiver.name, clients[i].name)) {
            if (!from_server) {
                strncpy(message, "[", sizeof message - strlen(message) - 1);
                strncat(message, sender.name, BUF_SIZE - 1);
                strncat(message, "] ", sizeof message - strlen(message) - 1);
            } else
                strncpy(message, "[SERVER] ", sizeof message - strlen(message) - 1);
            strncat(message, buffer, sizeof message - strlen(message) - 1);
            write_client(clients[i].sock, message);
        }
    }

}

static int init_connection(void) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);  // Initializing the socket : AF_INET = IPv4, SOCK_STREAM : TCP
    SOCKADDR_IN sin = {0};

    if (sock == INVALID_SOCKET) {   // if socket() returned -1
        perror("socket()");
        exit(errno);
    }

    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(PORT);
    sin.sin_family = AF_INET;

    if (bind(sock, (SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR) {
        perror("bind()");
        exit(errno);
    }

    if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR) {
        perror("listen()");
        exit(errno);
    }

    return sock;
}

static int str_equals(char *str1, char *str2) {
    if (!strcmp(str1, str2))
        return 1;
    return 0;
}

static void end_connection(int sock) {
    closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer) {
    int n = 0;

    if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0) {
        perror("recv()");
        /* if recv error we disonnect the client */
        n = 0;
    }

    buffer[n] = 0;

    return n;
}

static void write_client(SOCKET sock, const char *buffer) {
    if (send(sock, buffer, strlen(buffer), 0) < 0) {
        perror("send()");
        exit(errno);
    }
}

int main() {
    run();
    return EXIT_SUCCESS;
}

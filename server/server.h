#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>

#elif defined (linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

#define CRLF        "\r\n"
#define PORT        5000
#define MAX_CLIENTS    100

#define BUF_SIZE    1024

#include "client.h"

static void run(void);

static int str_equals(char *, char *);

static int init_connection(void);

static void end_connection(int sock);

static int read_client(SOCKET sock, char *buffer);

static void write_client(SOCKET sock, const char *buffer);

static void
send_message_to_all_clients(Client *clients, Client client, int actual, const char *buffer, char from_server);

static void send_message_to_client(Client *clients, Client sender, Client receiver, int actual, const char *buffer,
                                   char from_server);

static void remove_client(Client *clients, int to_remove, int *actual);

static void clear_clients(Client *clients, int actual);

#endif /* guard */

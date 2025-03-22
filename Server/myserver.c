#include "server.h"
#define LOG "files.log"
struct client_info
{
    SOCKET client;
    struct sockaddr_storage clientaddress;
    socklen_t clientlen;
    struct client_info *next;
};
static struct client_info *main_list = 0;
struct client_info *get_client(SOCKET s)
{
    struct client_info *temp = main_list;
    while (temp)
    {
        if (temp->client == s)
        {
            return temp;
        }
        temp = temp->next;
    }
    struct client_info *new = malloc(sizeof(*new));
    memset(new, 0, sizeof(struct client_info));
    new->next = main_list;
    main_list = new;
    return new;
}

void drop_client(SOCKET s)
{
    if (!get_client(s))
    {
        printf("NO SUCH CLIENT PRESENT ... \n");
        return;
    }
    struct client_info *next = main_list;
    while (next)
    {
        struct client_info *nextnode = next->next;
    }
}

SOCKET create_socket(char *host, char *port)
{
    struct addrinfo hints, *server_address;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(host, port, &hints, &server_address))
    {
        fprintf(stderr, "ERROR ON SETTING THE ADDRESS ........ \n");
        return 0;
    }
    char address[1024];
    char service[1024];
    printf("Getting Address .... \n");
    if (getnameinfo(server_address->ai_addr,
                    server_address->ai_addrlen, address,
                    sizeof(address), service, sizeof(service),
                    NI_NUMERICHOST))
    {
        fprintf(stderr, "ERROR \n");
        return 0;
    }
    printf("The address is %s and teh server is %s \n", address, service);
    printf("Creating Socket.... \n");
    SOCKET s = socket(server_address->ai_family, server_address->ai_socktype, server_address->ai_protocol);
    if (!ISSOCKETVALID(s))
    {
        fprintf(stderr, "ERROR-- %d \n", GETSOCKETERROR());
        return 0;
    }
    printf("Binding Socket .... \n");
    if (bind(s, server_address->ai_addr, server_address->ai_addrlen) != 0)
    {
        fprintf(stderr, "Connection Error  --- %d \n", GETSOCKETERROR());
        return 0;
    }
    printf("Listengin .... \n");

    if (listen(s, 10) < 0)
    {
        fprintf(stderr, "LISTENING ERROR ... \n");
        return 0;
    }
    return s;
}
void send_data(SOCKET s)

{
    printf("Sendign data .... \n");
    FILE *fp = fopen("../HTML/index.html", "r");
    char buffer[1024];
    const char *data = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: text/html\r\n"
                       "Transfer-Encoding: chunked\r\n\r\n";
    send(s, data, strlen(data), 0);
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0)
    {
        char chunk_size[30];
        sprintf(chunk_size, "%X\r\n", (unsigned int)bytes_read);
        send(s, chunk_size, strlen(chunk_size), 0);
        send(s, buffer, bytes_read, 0);
        send(s, "\r\n", 2, 0);
    }
    char *last_part = "0\r\n\r\n";
    send(s, last_part, 5, 0);
}

int main()
{
    SOCKET s;
    s = create_socket("127.0.0.1", "8080");
    if (!ISSOCKETVALID(s))
    {
        fprintf(stderr, "ERROR ON CONNECTION \n");
        return 1;
    }
    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(s, &reads);
    SOCKET max = s;
    while (1)
    {
        fd_set master = reads;
        if (select(max + 1, &reads, 0, 0, 0) < 0)
        {
            fprintf(stderr, "Nothing to connect at teh moment .. \n");
            exit(1);
        }
        if (FD_ISSET(s, &master))
        {
            printf("Reached till here \n");
            struct client_info *clients = get_client(-1);

            clients->client = accept(s, (struct sockaddr *)&(clients->clientaddress), &clients->clientlen);
            if (!ISSOCKETVALID(clients->client))
            {
                fprintf(stderr, "NEW CONNECTION :: ERROR\n");
                return 0;
            }
            if (clients->client > max)
                max = clients->client;
            FD_SET(clients->client, &reads);
        }
        struct client_info *temp = main_list;
        while (temp)
        {
            printf("the socket for this is %d \n", temp->client);
            if (FD_ISSET(temp->client, &reads))
            {
                send_data(temp->client);
                printf("html send to the client ... \n");
            }
            temp = temp->next;
        }
    }

    CLOSESOCKET(s);
    printf("Completed ... \n");
}

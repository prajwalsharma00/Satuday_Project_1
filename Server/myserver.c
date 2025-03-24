#include "server.h"
#define LOG "files.csv"
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
void store_it(char *request)
{
    FILE *fp = fopen("log.csv", "w");
    fseek(fp, 0, SEEK_SET);
    if (!fp)
    {
        printf("Unable to opne the file... \n");
        return;
    }
    char *new_data = strstr(request, "\r\n\r\n");
    char *actual_data;
    strncpy(actual_data, new_data, strlen(new_data) - 1);
    char *saveptr1, *saveptr2;
    if (actual_data)
    {
        char *token = strtok_r(actual_data + 5, ",", &saveptr1);
        while (token != NULL)
        {
            fprintf(stdout, "%s\n", token);
            char *data = strtok_r(token, ":", &saveptr2);
            while (data != NULL)
            {
                fprintf(fp, "%s,\t", data);

                printf("%s\n", data);
                data = strtok_r(NULL, ":", &saveptr2);
            }
            fprintf(fp, "\n");
            token = strtok_r(NULL, ",", &saveptr1);
        }
    }
    else
    {
        fputs(request, fp);
    }

    fclose(fp);
}

int numberofnodes(struct client_info *temp)
{
    int value = 0;
    while (temp)
    {
        value++;
        temp = temp->next;
    }
    return value;
}
void drop_client(struct client_info *client)
{
    CLOSESOCKET(client->client);
    struct client_info **p = &main_list;
    while (*p)
    {
        if (*p == client)
        {
            *p = client->next;
            free(client);
            return;
        }
        p = &(*p)->next;
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
char *sendresponse(char *data)
{
    char *response = malloc(1024);

    sprintf(response,
            "HTTP/1.1 200 OK\r\n"
            "Server: Prajwal_C_Server\r\n"
            "Content-Type: application/json\r\n"
            "\r\n"
            "{\"status\": \"%s\"}",
            data);

    return response;
}
void send_post_data(SOCKET s, char *path)
{
    char *response;
    if (strstr(path, "submit"))
    {

        response = sendresponse("Sucess");
    }
    else
    {
        printf("the response is %s \n", response);
        response = sendresponse("failure");
    }
    printf("The response is %s \n", response);

    send(s, response, strlen(response), 0);
    free(response);
}

void send_data(SOCKET s, char *path)

{
    printf("Sendign data .... \n");
    char full_path[1024];
    sprintf(full_path, "../HTML/%s", path);
    FILE *fp = fopen(full_path, "r");

    if (!fp)
    {
        printf("NO such file is presnt .. \n");
        const char *data = "HTTP/1.1 404 Not Found\r\n"
                           "Server: Prajwal_C_Server\r\n"
                           "Content-Type: text/html\r\n"
                           "Content-length: 16\r\n\r\n";
        send(s, data, strlen(data), 0);
        send(s, "Page Not Found!\r\n", 18, 0);
        return;
    }
    fseek(fp, 0, SEEK_SET);
    char buffer[1024];
    const char *data = "HTTP/1.1 200 OK\r\n"
                       "Server: Prajwal_C_Server\r\n"
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
void send_csv(SOCKET s)
{

    FILE *fp = fopen("log.csv", "r");
    if (!fp)
    {
        char *response = "HTTP/1.1 500 Internal Server Error \r\n\r\n";
        send(s, response, strlen(response), 0);
        return;
    }
    fseek(fp, 0, SEEK_END);
    size_t data_size = ftell(fp);
    printf("The size of cdv is %ld \n", data_size);
    fseek(fp, 0, SEEK_SET);

    char response[1024];
    sprintf(response,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/csv\r\n"
            "Server: Prajwal_C_Server\r\n"
            "Content-Disposition: attachment; filename=\"log.csv\"\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n",
            data_size); // End of headers
    printf("The response is %s \n", response);
    send(s, response, strlen(response), 0);
    char data[1024];
    size_t bytes_read;
    while ((bytes_read = fread(data, 1, sizeof(data), fp)) > 0)
    {
        printf("The data is %s \n", data);
        if (send(s, data, bytes_read, 0) < 0)
        {
            perror("Error in sending the data .. \n");
            break;
        }
    }
    fclose(fp);
}
void parserequest(char *request, SOCKET s, char *path)
{
    if (strstr(request, "GET"))

    {
        if (strstr(request, "download"))
        {
            printf("SEndign the csv file");

            send_csv(s);
            return;
        }
        else
        {
            send_data(s, path);
            return;
        }
    }
    else
    {
        store_it(request);

        send_post_data(s, path);
        return;
    }
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
        struct timeval ts;
        ts.tv_sec = 0;
        ts.tv_usec = 500000;
        if (select(max + 1, &master, 0, 0, &ts) < 0)
        {
            fprintf(stderr, "Nothing to connect at teh moment .. \n");
            break;
        }
        if (FD_ISSET(s, &master))
        {
            printf("New Connection is established.... \n");
            struct client_info *clients = get_client(-1);

            clients->client = accept(s, (struct sockaddr *)&(clients->clientaddress), &clients->clientlen);
            printf("The new client excepted is %d \n", clients->client);
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
        printf("the number of connection at the node is %d \n", numberofnodes(temp));
        while (temp)
        {

            if (FD_ISSET(temp->client, &master))
            {
                printf("int the f statenmet .. \n");
                printf("The ready client is %d \n", temp->client);
                char Client_request[2048];
                int recv_data = recv(temp->client, Client_request, sizeof(Client_request), 0);
                if (recv_data <= 0)
                {
                    printf("Client disconnected: %d\n", temp->client);

                    CLOSESOCKET(temp->client);
                    FD_CLR(temp->client, &reads);
                    drop_client(temp);
                    // Remove from main set
                    // remove from main_list too
                }
                printf("The client request is %s \n", Client_request);
                char *path = strstr(Client_request, "/");
                char *end_path = strstr(path, " HTTP/1.1");
                int length = end_path - (path + 1);
                char new_path[1024];
                strncpy(new_path, path + 1, length);
                printf("the length is %d \n", length);
                if (length == 0)
                {
                    parserequest(Client_request, temp->client, "index.html");
                }
                else
                {
                    printf("teh length is %d \n", length);

                    printf("The path is %s \n", new_path);
                    parserequest(Client_request, temp->client, new_path);
                }

                printf("html send to the client ... \n");
            }

            temp = temp->next;
        }
    }

    struct client_info *temp = main_list;
    struct client_info *next;
    while (temp)
    {

        next = temp->next;
        drop_client(temp);
        temp = next;
    }

    CLOSESOCKET(s);
    printf("Completed ... \n");
}

#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#define SOCKET int
#define ISSOCKETVALID(s) (s > 0)
#define CLOSESOCKET(s) close(s)
#define GETSOCKETERROR() errno
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
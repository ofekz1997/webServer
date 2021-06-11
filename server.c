#include "segel.h"
#include "request.h"
#include "threadPool.h"
#include <string.h>

//
// server.c: A very, very simple web server
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

//./server [portnum] [threads] [queue_size] [schedalg]
void getargs(int *port, int *poolSize, int *maxRequests, SchedAlg *schedAlg, int argc, char *argv[])
{
    if (argc < 5)
    {
        fprintf(stderr, "Usage: %s <port> <threads> <queue_size> <schedalg> \n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    *poolSize = atoi(argv[2]);
    *maxRequests = atoi(argv[3]);
    if (strcmp(argv[4], "block") == 0)
    {
        *schedAlg = BLOCK;
    }
    else if (strcmp(argv[4], "dt") == 0)
    {
        *schedAlg = DROP_TAIL;
    }
    else if (strcmp(argv[4], "dh") == 0)
    {
        *schedAlg = DROP_HEAD;
    }
    else
    {
        *schedAlg = RANDOM_DROP;
    }
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port = 8003, clientlen, poolSize = 2, maxRequests = 7;
    SchedAlg schedAlg = RANDOM_DROP;
    struct sockaddr_in clientaddr;

    getargs(&port, &poolSize, &maxRequests, &schedAlg, argc, argv);
    ThreadPool pool = ThreadPoolCreate(poolSize, maxRequests, schedAlg, pthread_self());

    listenfd = Open_listenfd(port);

    srand(time(NULL));
    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *)&clientlen);
        ThreadPoolAddRequest(pool, connfd, Time_GetTimeval());
    }

    ThreadPoolDestroy(pool);
}

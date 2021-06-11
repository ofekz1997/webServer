#include "threadPool.h"



typedef struct ThreadStat_t
{
    pthread_t thread;
    int id;
    int total_count;
    int static_req;
    int dynamic_req;
    ThreadPool pool; 
} ThreadStat;
 

struct Pool_t
{
    size_t poolSize;
    size_t maxRequest;
    size_t TotalRequests;
    SchedAlg schedAlg;
    List waitingRequests;
    ThreadStat* threadArray;

    pthread_mutex_t mutex;
    pthread_cond_t condCanDequeue;
    pthread_cond_t condBlock;
};

static char *fillBufferHdr(ThreadStat *stat, Request req)
{
    char *buf = malloc(MAXBUF);

    sprintf(buf, "Stat-Req-Arrival:: %lu.%06lu\r\n", req->arrival.tv_sec, req->arrival.tv_usec);
    if(req->pickup.tv_usec  < req->arrival.tv_usec)
    {
        sprintf(buf, "%sStat-Req-Dispatch:: %lu.%06lu\r\n", buf, 
            req->pickup.tv_sec - req->arrival.tv_sec-1, 1000000+ req->pickup.tv_usec - req->arrival.tv_usec);
    }
    else
    {
        sprintf(buf, "%sStat-Req-Dispatch:: %lu.%06lu\r\n", buf, 
            req->pickup.tv_sec - req->arrival.tv_sec, req->pickup.tv_usec - req->arrival.tv_usec);
    }
    

    sprintf(buf, "%sStat-Thread-Id:: %d\r\n", buf, stat->id);
    sprintf(buf, "%sStat-Thread-Count:: %d\r\n", buf, stat->total_count);
    return buf;
}

static void *HandleRequest(void *data)
{
    ThreadStat *thread = (ThreadStat *)data;
    ThreadPool pool = thread->pool;
    Request req = NULL;
    while (true)
    {
        pthread_mutex_lock(&(pool->mutex));
        while (listGetSize(pool->waitingRequests) == 0)
        {
            pthread_cond_wait(&(pool->condCanDequeue), &(pool->mutex));
        }
        req = listDequeue(pool->waitingRequests);
        pthread_mutex_unlock(&(pool->mutex));

        thread->total_count++;
        char *buf = fillBufferHdr(thread, req);

        requestHandle(req->fd, buf, &thread->static_req, &thread->dynamic_req); 
        
        free(buf);

        pthread_mutex_lock(&(pool->mutex));
        --(pool->TotalRequests);
        pthread_cond_signal(&(pool->condBlock));
        pthread_mutex_unlock(&(pool->mutex));

        Close(req->fd);
    }
    return NULL;
}

ThreadPool ThreadPoolCreate(size_t poolSize, size_t maxRequest, SchedAlg schedAlg, pthread_t mainThreadID)
{
    ThreadPool new_pool = (ThreadPool)malloc(sizeof(*new_pool));
    if (new_pool == NULL)
    {
        return NULL;
    }
    new_pool->TotalRequests = 0;
    new_pool->poolSize = poolSize;
    new_pool->maxRequest = maxRequest;
    new_pool->schedAlg = schedAlg;
    new_pool->waitingRequests = listCreate();

    new_pool->threadArray = (ThreadStat*)malloc(poolSize*sizeof(*(new_pool->threadArray)));

    pthread_mutex_init(&(new_pool->mutex), NULL);
    pthread_cond_init(&(new_pool->condCanDequeue), NULL);
    pthread_cond_init(&(new_pool->condBlock), NULL);


    for (int i = 0; i < poolSize; i++)
    {
        new_pool->threadArray[i].id = i;
        new_pool->threadArray[i].dynamic_req = 0;
        new_pool->threadArray[i].static_req = 0;
        new_pool->threadArray[i].total_count = 0;
        new_pool->threadArray[i].pool = new_pool;

        pthread_create(&(new_pool->threadArray[i].thread), NULL, HandleRequest, &(new_pool->threadArray[i]));
    }


    return new_pool;
}

void ThreadPoolDestroy(ThreadPool pool)
{
    listDestroy(pool->waitingRequests);
    for (size_t i = 0; i < pool->poolSize; i++)
    {
        pthread_cancel(pool->threadArray[i].thread);
    }

    pthread_mutex_destroy(&(pool->mutex));
    pthread_cond_destroy(&(pool->condCanDequeue));
    pthread_cond_destroy(&(pool->condBlock));
    
    free(pool->threadArray);
    free(pool);
}

void ThreadPoolAddRequest(ThreadPool pool, int fd, struct timeval arrival)
{
    if(pool->TotalRequests >= pool->maxRequest)
    {
        switch (pool->schedAlg)
        {
        case DROP_TAIL:
        {
            Close(fd);
            break;
        }
        case DROP_HEAD:
        {
            if(listGetSize(pool->waitingRequests) == 0)
            {
                Close(fd);
            }
            else
            {
                pthread_mutex_lock(&(pool->mutex));
                    Request req = listDequeue(pool->waitingRequests);
                    Close(req->fd); 
                    free(req);
                    listEnqueue(pool->waitingRequests, fd, arrival);
                pthread_mutex_unlock(&(pool->mutex));
            }
            break;
        }
        case BLOCK:
        {

            pthread_mutex_lock(&(pool->mutex));
                while (pool->TotalRequests >= pool->maxRequest) 
                {
                    pthread_cond_wait(&(pool->condBlock), &(pool->mutex));
                }

                listEnqueue(pool->waitingRequests,fd, arrival);
                ++pool->TotalRequests;
                pthread_cond_signal(&(pool->condCanDequeue));
            pthread_mutex_unlock(&(pool->mutex));

            break;
        } 
        case RANDOM_DROP:
        {
            pthread_mutex_lock(&(pool->mutex));
                pool->TotalRequests -= listDrop(pool->waitingRequests,0.25);
                listEnqueue(pool->waitingRequests,fd, arrival);
                ++pool->TotalRequests;
            pthread_mutex_unlock(&(pool->mutex));
            break;
        }
        default:
            break;
        }
    }
    else
    {
        pthread_mutex_lock(&(pool->mutex));
            listEnqueue(pool->waitingRequests,fd, arrival);
            ++pool->TotalRequests;
            pthread_cond_signal(&(pool->condCanDequeue));
        pthread_mutex_unlock(&(pool->mutex));
    }
}
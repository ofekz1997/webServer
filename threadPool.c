#include "threadPool.h"



typedef struct ThreadStat_t
{
    pthread_t thread;
    int id;
    int total_count;
    int static_req;
    int dynamic_req;
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


typedef struct HandleRequest_struct
{
    struct Pool_t *pool;
    ThreadStat *threadStat;

} *HandleRequest_struct;

static void* HandleRequest(void *data)
{
    HandleRequest_struct cur_st  = (HandleRequest_struct)data;
    ThreadPool cur_pool = cur_st->pool;
    ThreadStat *st =  cur_st->threadStat;
    Request req = NULL;
    while(true)
    {
        pthread_mutex_lock(&(cur_pool->mutex));
            while (listGetSize(cur_pool->waitingRequests) == 0) 
            {
                pthread_cond_wait(&(cur_pool->condCanDequeue), &(cur_pool->mutex));
            }
            req = listDequeue(cur_pool->waitingRequests);
        pthread_mutex_unlock(&(cur_pool->mutex));

        requestHandle(req->fd); //TODO change requestHandle, need to update ThreadStat and change print.
        printf("finish job\n");

        pthread_mutex_lock(&(cur_pool->mutex));
            --cur_pool->TotalRequests;
            pthread_cond_signal(&(cur_pool->condBlock));
        pthread_mutex_unlock(&(cur_pool->mutex));

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
        HandleRequest_struct st = malloc(sizeof(*st));
        st->threadStat = &(new_pool->threadArray[i]);
        st->pool = new_pool;
        pthread_create(&(new_pool->threadArray[i].thread), NULL, HandleRequest, st);
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

void ThreadPoolAddRequest(ThreadPool pool, int fd, double arrival)
{
    if(pool->TotalRequests >= pool->maxRequest)
    {
        switch (pool->schedAlg)
        {
        case DROP_TAIL:
        {
            printf("in DROP_TAIL\n");
            Close(fd);
            break;
        }
        case DROP_HEAD:
        {
            printf("in DROP_HEAD\n");
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
            printf("in BLOCK\n");

            pthread_mutex_lock(&(pool->mutex));
                while (pool->TotalRequests >= pool->maxRequest) 
                {
                    pthread_cond_wait(&(pool->condBlock), &(pool->mutex));
                }

                listEnqueue(pool->waitingRequests,fd, arrival);
                printf("insert\n");
                ++pool->TotalRequests;
                pthread_cond_signal(&(pool->condCanDequeue));
            pthread_mutex_unlock(&(pool->mutex));

            printf("out BLOCK\n");
            break;
        } 
        case RANDOM_DROP:
        {
            pthread_mutex_lock(&(pool->mutex));
                pool->TotalRequests -= listDrop(pool->waitingRequests,0.25);
                listEnqueue(pool->waitingRequests,fd, arrival);
                printf("insert\n");
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
        printf("insert\n");
    }
}
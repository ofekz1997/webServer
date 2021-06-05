#include "threadPool.h"
struct Pool_t
{
    size_t poolSize;
    size_t maxRequest;
    SchedAlg schedAlg;
    List waitingRequests;
    List inProgressRequests;
    pthread_t* threadArray;
};

static void* HandleRequest(void *pool)
{
    ThreadPool cur_pool = (ThreadPool)pool;
    int fd = -1;
    while(true)
    {
        fd = listDequeue(cur_pool->waitingRequests);
        listEnqueue(cur_pool->inProgressRequests,fd);
        requestHandle(fd);
        //TODO if we get new task here?
        listRemove(cur_pool->inProgressRequests,fd);
        Close(fd);
    }
    return NULL;
}

ThreadPool ThreadPoolCreate(size_t poolSize, size_t maxRequest, SchedAlg schedAlg)
{
    ThreadPool new_pool = (ThreadPool)malloc(sizeof(*new_pool));
    if (new_pool == NULL)
    {
        return NULL;
    }
    new_pool->poolSize = poolSize;
    new_pool->maxRequest = maxRequest;
    new_pool->schedAlg = schedAlg;
    new_pool->waitingRequests = listCreate();
    new_pool->inProgressRequests = listCreate();
    new_pool->threadArray = (pthread_t*)malloc(poolSize*sizeof(pthread_t));
    for (size_t i = 0; i < poolSize; i++)
    {
        pthread_create(&(new_pool->threadArray[i]), NULL, HandleRequest, new_pool);
    }
    return new_pool;
}

void ThreadPoolDestroy(ThreadPool pool)
{
    listDestroy(pool->waitingRequests);
    listDestroy(pool->inProgressRequests);
    for (size_t i = 0; i < pool->poolSize; i++)
    {
        pthread_cancel(pool->threadArray[i]);
    }
    
    free(pool->threadArray);
    free(pool);
}

void ThreadPoolAddRequest(ThreadPool pool,int fd)
{
    
    if(listGetSize(pool->inProgressRequests) + listGetSize(pool->waitingRequests) >  pool->maxRequest)
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
            
            break;
        }
        
        default:
            break;
        }
    }
    else
    {
        listEnqueue(pool->waitingRequests,fd);
    }
}
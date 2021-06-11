#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include "list.h"
#include <pthread.h>
#include "request.h"
#include "segel.h"

typedef struct Pool_t* ThreadPool;

typedef enum SchedAlg_t {
    BLOCK,
    DROP_TAIL,
    DROP_HEAD,
    RANDOM_DROP
} SchedAlg;



ThreadPool ThreadPoolCreate(size_t poolSize, size_t maxRequest, SchedAlg schedAlg, pthread_t mainThreadID);
void ThreadPoolDestroy(ThreadPool pool);
void ThreadPoolAddRequest(ThreadPool pool,int fd, double arrival);


#endif // THREADS_POOL_H_
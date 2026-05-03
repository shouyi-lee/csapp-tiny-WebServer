#include "config.h"
#include "rio_io.h"
#include "pool.h"

#include <semaphore.h>
#include <pthread.h>

static task_t task_table[TASK_NUM];
static customer_t customer_table[CUSTOMER_NUM];

static size_t head, top;
static sem_t task_free;
static sem_t task_avalaible;

static pthread_mutex_t mutex;

static fd_set serve_fds;

ssize_t pool_init()
{

}

void *task_register(void *args)
{
    for(;;)
        for (size_t i = 0; i < CUSTOMER_NUM; i++)
        {
            if (customer_table[i].alive == 1)
                FD_SET(customer_table[i].rio.rio_fd, &serve_fds);
        }
}

task_t task_fetch()
{
    sem_wait(&task_avalaible);

    pthread_mutex_lock(&mutex);
    task_t task = task_table[head++];
    pthread_mutex_unlock(&mutex);

    sem_post(&task_free);

    return task;
}

ssize_t task_return(task_t *task)
{
    sem_wait(&task_free);

    if (task->customer->alive == 0)
        customer_delete(task->customer);

    return 0;
}

ssize_t customer_add(int fd, void *client_info, size_t client_info_len)
{

}

ssize_t customer_delete(customer_t *customer)
{

}

ssize_t pool_deinit()
{

}
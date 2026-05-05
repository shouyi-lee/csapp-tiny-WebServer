#include "config.h"
#include "rio_io.h"
#include "pool.h"

#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <sys/select.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>

static task_t task_table[TASK_NUM];
static customer_t customer_table[CUSTOMER_NUM];

static size_t head, top;
static pthread_mutex_t mutex;

static sem_t task_free;
static sem_t task_avalaible;
static sem_t customer_space_free;
static sem_t customer_space_used;

static fd_set serve_fds;

static pthread_t register_pth;

static int wake_fd;

ssize_t pool_init()
{
    memset(&task_table, 0, sizeof(task_table[0]) * TASK_NUM);
    memset(&customer_table, 0, sizeof(customer_table[0]) * CUSTOMER_NUM);

    for (size_t i = 0; i < CUSTOMER_NUM; i++)
    {
        pthread_mutex_init(&customer_table[i].mutex, NULL);
    }

    head = top = 0;

    pthread_mutex_init(&mutex, NULL);

    sem_init(&task_free, 0, TASK_NUM);
    sem_init(&task_avalaible, 0, 0);
    sem_init(&customer_space_free, 0, CUSTOMER_NUM);
    sem_init(&customer_space_used, 0, 0);

    wake_fd = eventfd(0, 0);

    pthread_create(&register_pth, NULL, task_search, NULL);
    pthread_detach(register_pth);

    return 0;
}

void *task_search(void *args)
{
    (void) args;
    for(;;)
    {
        fd_set save;
        FD_ZERO(&serve_fds);
        FD_SET(wake_fd, &serve_fds);

        for (size_t i = 0; i < CUSTOMER_NUM; i++)
        {
            customer_t *customer = &customer_table[i];
            pthread_mutex_lock(&customer->mutex); 

            if (customer->used == 1 && customer->dealing == 0)
                RIO_SET(&customer->rio, &serve_fds);
                
            pthread_mutex_unlock(&customer->mutex);
        }

        save = serve_fds;
        (void) save;

        struct timeval wait_time = {.tv_sec = 1};
        int res = select(FD_SETSIZE, &serve_fds, NULL, NULL, &wait_time);

        if (res == 0)
        {
            struct timespec nowtime;
            clock_gettime(CLOCK_MONOTONIC, &nowtime);

            for (size_t i = 0; i < CUSTOMER_NUM; i++)
            {
                pthread_mutex_lock(&customer_table[i].mutex);
                if (customer_table[i].used == 0
                || (customer_table[i].used == 1
                && customer_table[i].dealing == 1))
                {
                    pthread_mutex_unlock(&customer_table[i].mutex);
                    continue;
                }

                if (nowtime.tv_sec - customer_table[i].last_active.tv_sec > 5)
                {
                    pthread_mutex_unlock(&customer_table[i].mutex);
                    customer_delete(&customer_table[i]);
                }
                pthread_mutex_unlock(&customer_table[i].mutex);
            }

            continue;
        } 
        else if (res < 0)
        {
            continue;
        }

        for (size_t i = 0; i < CUSTOMER_NUM; i++)
        {
            if (RIO_ISSET(&customer_table[i].rio, &serve_fds))
                task_register(&customer_table[i]);

            if (FD_ISSET(wake_fd, &serve_fds))
            {
                uint64_t u64;
                ssize_t res = read(wake_fd, &u64, 8);
                (void) res;
                break;
            }
        }
    }

    return NULL;
}

ssize_t task_register(customer_t *customer)
{
    sem_wait(&task_free);

    pthread_mutex_lock(&customer->mutex);
    customer->dealing = 1;
    pthread_mutex_unlock(&customer->mutex);

    pthread_mutex_lock(&mutex);
    top = (top + 1) % TASK_NUM;
    task_t task = {.customer = customer};
    task_table[top] = task;
    pthread_mutex_unlock(&mutex);

    sem_post(&task_avalaible);

    return 0;
}

task_t task_fetch()
{
    sem_wait(&task_avalaible);

    pthread_mutex_lock(&mutex);
    head = (head + 1) % TASK_NUM;
    task_t task = task_table[head];
    pthread_mutex_unlock(&mutex);

    sem_post(&task_free);

    return task;
}

ssize_t task_return(task_t task)
{
    if (task.keep_alive == 0)
        customer_delete(task.customer);
    else
    {
        pthread_mutex_lock(&task.customer->mutex);
        task.customer->dealing = 0;
        clock_gettime(CLOCK_MONOTONIC, &task.customer->last_active);
        pthread_mutex_unlock(&task.customer->mutex);
    }

    return 0;
}

ssize_t customer_add(int fd, void *client_info, size_t client_info_len)
{
    sem_wait(&customer_space_free);

    size_t index;
    for (index = 0; index < CUSTOMER_NUM; index++)
    {
        customer_t *this_customer = &customer_table[index];
        pthread_mutex_lock(&this_customer->mutex);
        if (this_customer->used == 0)
            break;
        else
            pthread_mutex_unlock(&this_customer->mutex);
    }
    customer_t *customer = &customer_table[index];

    customer->used = 1;
    customer->dealing = 0;
    clock_gettime(CLOCK_MONOTONIC, &customer->last_active);
    rio_init(&customer->rio, fd);
    memcpy(&customer->client_info, client_info, client_info_len);

    pthread_mutex_unlock(&customer->mutex);
    sem_post(&customer_space_used);

    uint64_t u64 = 1;
    ssize_t res = write(wake_fd, &u64, 8);
    (void) res;

    return 0;
}

ssize_t customer_delete(customer_t *customer)
{
    sem_wait(&customer_space_used);
    pthread_mutex_lock(&customer->mutex);
    
    customer->used = 0;
    customer->dealing = 0;
    rio_deinit(&customer->rio);

    pthread_mutex_unlock(&customer->mutex);
    sem_post(&customer_space_free);

    return 0;
}

ssize_t pool_deinit()
{
    pthread_cancel(register_pth);

    return 0;
}
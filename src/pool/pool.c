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
#include <sys/epoll.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>

static task_t task_table[TASK_NUM];
static customer_t customer_table[CUSTOMER_NUM];

static size_t head, tail;
static pthread_mutex_t mutex;

static sem_t task_free;
static sem_t task_available;
static sem_t customer_space_slot;
static sem_t customer_space_occupied;

static int epoll_fd;

static pthread_t register_pth;

ssize_t pool_init()
{
    memset(&task_table, 0, sizeof(task_table[0]) * TASK_NUM);
    memset(&customer_table, 0, sizeof(customer_table[0]) * CUSTOMER_NUM);

    for (size_t i = 0; i < CUSTOMER_NUM; i++)
        pthread_mutex_init(&customer_table[i].mutex, NULL);

    head = tail = 0;

    pthread_mutex_init(&mutex, NULL);

    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd < 0) return -1;

    sem_init(&task_free, 0, TASK_NUM);
    sem_init(&task_available, 0, 0);
    sem_init(&customer_space_slot, 0, CUSTOMER_NUM);
    sem_init(&customer_space_occupied, 0, 0);

    pthread_create(&register_pth, NULL, customer_listen, NULL);
    pthread_detach(register_pth);

    return 0;
}

void *customer_listen(void *args)
{
    (void)args;
    struct epoll_event events[TASK_NUM];

    for (;;)
    {
        ssize_t wait_rc = epoll_wait(epoll_fd, events, TASK_NUM, TIME_OUT);

        if (wait_rc == 0) continue;
        else if (wait_rc < 0)
        {
            if (errno == EINTR) continue;
            exit(-1);
        }

        for (size_t i = 0; i < (size_t)wait_rc; i++)
            task_register(events[i].data.ptr);
    }
}

ssize_t task_register(customer_t *customer)
{
    sem_wait(&task_free);

    pthread_mutex_lock(&customer->mutex);
    customer->busy = 1;
    pthread_mutex_unlock(&customer->mutex);

    pthread_mutex_lock(&mutex);
    tail = (tail + 1) % TASK_NUM;
    task_t task = {.customer = customer};
    task_table[tail] = task;

    retry: ssize_t rc = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, task.customer->rio.rio_fd, NULL);
    if (rc < 0)
    {
        if (errno == EINTR)
            goto retry;
        exit(-1);
    }

    pthread_mutex_unlock(&mutex);
    sem_post(&task_available);

    return 0;
}

task_t task_acquire()
{
    sem_wait(&task_available);

    pthread_mutex_lock(&mutex);
    head = (head + 1) % TASK_NUM;
    task_t task = task_table[head];
    pthread_mutex_unlock(&mutex);

    sem_post(&task_free);

    return task;
}

ssize_t task_return(task_t task)
{
    if (task.reuse == 0)
        customer_delete(task.customer);
    else
    {
        pthread_mutex_lock(&task.customer->mutex);
        task.customer->busy = 0;
        pthread_mutex_unlock(&task.customer->mutex);

        struct epoll_event ev;
        ev.data.ptr = task.customer;
        ev.events = EPOLLIN | EPOLLRDHUP;
        retry: ssize_t rc = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, task.customer->rio.rio_fd, &ev);
        if (rc < 0)
        {
            if (errno == EINTR)
                goto retry;
            exit(-1);
        }
    }

    return 0;
}

ssize_t customer_add(int fd)
{
    sem_wait(&customer_space_slot);

    size_t index;
    customer_t *this_customer;
    for (index = 0; index < CUSTOMER_NUM; index++)
    {
        this_customer = &customer_table[index];
        pthread_mutex_lock(&this_customer->mutex);
        if (this_customer->occupied == 0)
            break;
        else
            pthread_mutex_unlock(&this_customer->mutex);
    }

    this_customer->occupied = 1;
    this_customer->busy = 0;
    rio_init(&this_customer->rio, fd);

    pthread_mutex_unlock(&this_customer->mutex);
    sem_post(&customer_space_occupied);

    struct epoll_event ev;
    ev.data.ptr = this_customer;
    ev.events = EPOLLIN | EPOLLRDHUP;
    retry: ssize_t rc = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    if (rc < 0)
    {
        if (errno == EINTR)
            goto retry;
        exit(-1);
    }

    return 0;
}

/*只能由task_return调用*/
ssize_t customer_delete(customer_t *customer)
{
    sem_wait(&customer_space_occupied);
    pthread_mutex_lock(&customer->mutex);
    
    customer->occupied = 0;
    customer->busy = 0;
    rio_deinit(&customer->rio);

    pthread_mutex_unlock(&customer->mutex);
    sem_post(&customer_space_slot);

    return 0;
}

ssize_t pool_destroy()
{
    pthread_cancel(register_pth);

    return 0;
}
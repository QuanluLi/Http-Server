#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>

class Mutex {
public:
    Mutex();
    ~Mutex();

    int lock();
    int unlock();

    pthread_mutex_t *GetMutex();
    bool IsLock();

private:
    pthread_mutex_t m_lock;
    volatile bool is_lock;  //为什么要声明成volatile
    volatile bool is_OK;
};

#endif
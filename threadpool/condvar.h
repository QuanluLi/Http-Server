#ifndef CONDVAR_H
#define CONDVAR_H

#include <pthread.h>


class CondVar {
public:
    CondVar();
    ~CondVar();

    void waitCond(pthread_mutex_t *mutex);
    void signal();
    void broadcast();

private:
    pthread_cond_t m_condvar;
    bool is_OK;
};

#endif
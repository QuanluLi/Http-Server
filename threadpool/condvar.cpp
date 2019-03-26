#include "condvar.h"
#include "../errexit.h"

#include <iostream>
using namespace std;

CondVar::CondVar() : is_OK(true) {
    if (pthread_cond_init(&m_condvar, NULL) != 0) {
        is_OK = false;
        errExit("condition variable init error");
    }
}

CondVar::~CondVar() {
    if(!is_OK) {
        //cout << "condition varibale state error.\n";
        return;
    }
    if(pthread_cond_destroy(&m_condvar) < 0) {
        errExit("conditon variable destroy error");
    }
}

void CondVar::waitCond(pthread_mutex_t *mutex) {
    if(!is_OK) {
        cout << "condition varibale state error.\n";
        return;
    }
    if(pthread_cond_wait(&m_condvar, mutex) != 0) {
        errExit("conditon variable wait error");
    }   
}

void CondVar::signal() {
    if(!is_OK) {
        cout << "condition varibale state error.\n";
        return;
    }
    if(pthread_cond_signal(&m_condvar) != 0) {
        errExit("conditon variable signal error");
    }
}

void CondVar::broadcast() {
    if(!is_OK) {
        cout << "condition varibale state error.\n";
        return;
    }
    if(pthread_cond_broadcast(&m_condvar) != 0) {
        errExit("conditon variable broadcast error");
    }
}
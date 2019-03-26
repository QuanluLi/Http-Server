#include "mutex.h"
#include "../errexit.h"

using namespace std;

Mutex::Mutex() : is_lock(false), is_OK(true) {
    int res = pthread_mutex_init(&m_lock, NULL);
    if(res != 0) {
        errExit("pthread mutex create error");
        is_OK = false;
    }
}

Mutex::~Mutex() {
    if(!is_OK) return;
    while(is_lock) unlock();
    int res = pthread_mutex_destroy(&m_lock);
    if(res < 0)
        errExit("pthread mutex destroy error");
}

int Mutex::lock() {
    if(!is_OK) {
        cout << "mutex state error.\n";
        return -1;
    }
    if(pthread_mutex_lock(&m_lock) != 0) {
        errExit("pthread mutex lock error");
        return -1;
    }
    is_lock = true;
    return 0;
}

int Mutex::unlock() {
    if(!is_OK) {
        cout << "mutex state error.\n";
        return -1;
    }
    if(pthread_mutex_unlock(&m_lock) != 0) {
        errExit("pthread mutex unlock error");
        return -1;
    }
    is_lock = false;
    return 0;
}

pthread_mutex_t *Mutex::GetMutex() {
    if(!is_OK) {
        cout << "mutex state error.\n";
        return NULL;
    }
    return &m_lock;
}

bool Mutex::IsLock() {
    return is_lock;
}
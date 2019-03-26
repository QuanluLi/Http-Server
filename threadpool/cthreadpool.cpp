#include "cthreadpool.h"
#include <stdio.h>

using namespace std;

void CTask::SetData(void *args) {
    this->m_args = args;
}

//静态变量初始化
deque<CTask *> CThreadPool::m_TaskList;
volatile bool CThreadPool::shutdown = false;
Mutex CThreadPool::m_condMutex;
Mutex CThreadPool::m_taskMutex;
CondVar CThreadPool::m_pthreadCond;

CThreadPool::CThreadPool(int threadNum) : m_iniThreadNum(threadNum) {
    m_threadList.clear();
    printf("I will create %d threads.\tfile: %s\tline: %d\n", threadNum, __FILE__, __LINE__);  //编译器内置的宏_FILE_在源文件中插入当前文件名，_LINE_在源文件中插入当前行号
    Create();
}

void *CThreadPool::ThreadFunc(void *threadData) {
    pthread_t tid = pthread_self(); //获得当前线程ID
    while(1) {
        m_condMutex.lock();

        //如果队列为空且未设退出标识，则等待新任务进入任务队列
        while(!shutdown && m_TaskList.empty()) {
            m_pthreadCond.waitCond(m_condMutex.GetMutex());
        }

        if(shutdown) {
            m_condMutex.unlock();
            printf("[tid: %lu]\texit\tfile: %s\tline: %d\n", tid, __FILE__, __LINE__);
            pthread_exit(NULL);
        }

        printf("[tid: %lu]\trun: \tfile: %s\tline: %d\n", tid, __FILE__, __LINE__);
        
        //取出一个任务并处理之
        m_taskMutex.lock();
        CTask *task = m_TaskList.front();
        m_TaskList.pop_front();

        m_taskMutex.unlock();
        m_condMutex.unlock();

        task->Run();    //执行任务
        delete task;
        printf("[tid: %lu]\tidle\tfile: %s\tline: %d\n", tid, __FILE__, __LINE__);
    }
    return NULL;
}

//向任务队里中添加任务
int CThreadPool::AddTask(CTask *task) {
    m_taskMutex.lock();
    m_TaskList.push_back(task);
    m_taskMutex.unlock();
    m_pthreadCond.signal();
    return 0;
}

//创建线程
int CThreadPool::Create() {
    m_threadList.clear();
    pthread_t pthread_id;
    for(int i = 0; i < m_iniThreadNum; i++) {
        pthread_create(&pthread_id, NULL, ThreadFunc, NULL);
        m_threadList.push_back(pthread_id);
    }
    return 0;
}

//停止所有线程
int CThreadPool::StopAll() {
    if(shutdown) return -1;
    printf("Now I will end all threads!\tfile: %s\tline: %d\n\n", __FILE__, __LINE__);

    //唤醒所有等待线程
    shutdown = true;
    m_pthreadCond.broadcast();

    //清除僵尸线程
    for(int i = 0; i < m_iniThreadNum; i++) {
        pthread_join(m_threadList[i], NULL);
    }

    m_threadList.clear();
    return 0;
}



//获得当前队列中的线程数
int CThreadPool::GetTaskSize() {
    return m_TaskList.size();
}

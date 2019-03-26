#ifndef __CTHREAD_POOL_H
#define __CTHREAD_POOL_H

#include <deque>
#include <vector>
#include <string>
#include <pthread.h>
#include <iostream>

#include "mutex.h"
#include "condvar.h"

/* 任务抽象类 */
class CTask {
public:
    CTask(void* (*fn_ptr)(void *), void *args = NULL, char *taskName = NULL) :
        m_fn_ptr(fn_ptr), m_args(args), m_TaskName(taskName){};
    virtual ~CTask() {};

    virtual int Run() = 0 ; //执行的具体任务
    void SetData(void *args);   //r任务数据

protected:
    void* (*m_fn_ptr)(void *);

    void *m_args; //要执行任务的具体数据
    char *m_TaskName; //任务名称  

private:
    CTask();
    
};

class CMyTask : public CTask {
public:
    CMyTask(void* (*fn_ptr)(void *), void *args = NULL, char *taskName = NULL) : CTask(fn_ptr, args, taskName) {}
    ~CMyTask() {}
    int Run() {
        (*m_fn_ptr)(m_args);
        return 0;
    }
};

class CThreadPool {
public:
    CThreadPool(int threadNum = 30);
    int AddTask(CTask *task);   //把任务添加到任务队列中
    int StopAll();  //所有线程退出
    int GetTaskSize();  //获取当前任务对列中的任务数

protected:
    static void *ThreadFunc(void *threadData);  //线程回调函数
    int Create();   //创建线程池中的线程

private:
    static std::deque<CTask *> m_TaskList; //任务
//volatile的作用：
// 1 不会在两个操作之间把volatile变量缓存在寄存器中。在多任务、中断、甚至setjmp环境下，变量可能被其他的程序改变，编译器自己无法知道，volatile就是告诉编译器这种情况。
// 2 不做常量合并、常量传播等优化，所以像下面的代码： 
// volatile int i = 1; 
// if (i > 0) ... 
// if的条件不会当作无条件真。 
// 3 对volatile变量的读写不会被优化掉。如果你对一个变量赋值但后面没用到，编译器常常可以省略那个赋值操作，然而对Memory Mapped IO的处理是不能这样优化的。 
    volatile static bool shutdown;  //线程退出标识  
    int m_iniThreadNum;   //线程池中启动的线程数
    std::vector<pthread_t> m_threadList;
    static Mutex m_condMutex;  //线程同步锁
    static Mutex m_taskMutex;
    static CondVar m_pthreadCond;    //线程同步条件变量
};

#endif
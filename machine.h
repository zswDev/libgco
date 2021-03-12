#ifndef __worker_h__
#define __worker_h__

#include<vector>
#include<map>
#include<atomic>
#include<mutex>
#include<thread>
#include<iostream>

using namespace std;

#include "aco.h"
#include "concurrentqueue.h"

#include "task.h"
#include "utils.h"
#include "mlist.h"

using namespace std;

typedef mlist<task*> atomic_queue;

enum M_STATE{
    wait,
    start,
    block,
    death,
};

class machine{
private:
    task* co = nullptr;       // 当前正在执行的co
    pthread_t thid = 0;

    bool work_stealing(int offset); // 偷取算法
    task* get_task();
public:
    aco_t* main_co = nullptr; // 记录当前线程的主要堆栈
    M_STATE state = M_STATE::start;
    mlist<task*>* p = nullptr;   // 绑定的p队列

    condx lock; // TODO 如何让他不可改

    machine(atomic_queue* p);

    void run();

    pthread_t get_tid();

    void resume_co();

    friend void* m_loop(void* args);
};
 
// extern moodycamel::ConcurrentQueue<machine*> m_idle;

extern thread_local machine* local_m;

//extern thread_local aco_t* local_main_co;

#endif
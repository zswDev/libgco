#ifndef __scheduler__h__
#define __scheduler__h__

#include<iostream>
#include<map>
#include <pthread.h>
#include <mutex>
#include <signal.h>

#include "machine.h"
#include "mlist.h"

using namespace std;

//#include <sched.h>
//#include <linux/futex.h>
//#include <sys/syscall.h>
// #define futex(addr1, op, val, rel, addr2, val3)   \
//     syscall(SYS_futex, addr1, op, val, rel, addr2, val3)
// TODO futex

typedef mlist<task*> atomic_queue;

extern int max_queue_len;

extern atomic<int> thief_work_len; // 偷取者数量

class scheduler{
public:
    atomic_queue global_tasks;
    vector<atomic_queue*> local_tasks;
    mlist<atomic_queue*> p_idle;

    mlist<machine*> m_idle;
    mlist<machine*> m_run;
    int p_count = 0;
    static vector<int> coprime_arr; // 互质数组

    scheduler();
    void init();
    friend void* background(void* args);
    bool join(mlist<task*>* p, machine* m = nullptr); //寻找可用的m 使其运行
};

extern scheduler* sched;

extern void go(function<void()>, task* co = nullptr);

#endif
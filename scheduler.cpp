#include<time.h>
#include<stdlib.h>
#include<sys/time.h>

#include <mutex>

#include "utils.h"
#include "machine.h"
#include "scheduler.h"
#include "co_io.h"
#include "uv_timer.h"

int max_queue_len = 8;
atomic<int> thief_work_len = {0};
vector<int> scheduler::coprime_arr = {};
vector<machine*> m_death = {};



void step_loop () {
    
    vector<machine*>().swap(m_death); //清0

    function<bool(machine* m)> func = [](machine* m)->bool{
            int kill_rc = pthread_kill(m->get_tid(), 0);

        if(kill_rc == ESRCH) { //指定的线程不存在或者是已经终止， 一般是co 执行失败了

            m_death.push_back(m);
            m->state = M_STATE::death;

            return false;
        } else if(kill_rc == EINVAL) { // 调用传递一个无用的信号

        }

        return true;
    };

    sched->m_run.filter(func); //检查m链表 

    for(machine* m : m_death) {
        sched->join(m->p);
    }

    //检测是否有任务未执行
    if(sched->p_idle.len() != 0 &&
        sched->global_tasks.len() != 0) {
        
        cout<<"notify"<<endl;

        atomic_queue* p = nullptr;
        while(sched->p_idle.pop(p)) { //唤醒全部p 或者部分p  
            if (p != nullptr) {
                sched->join(p);
            }
        }
    }

    // for(atomic_queue* q1 : sched->local_tasks) {
    //     cout<<q1->size_approx()<<endl;
    // }
    // cout<<sched->global_tasks.size_approx()<<endl;

};

condx back_cond;

void* background(void*){
    timer* t = new timer();
    t->main_add(step_loop, 1, 1);

    cout<<"open"<<endl;
    back_cond.notify(); //通知主线程继续

    co_loop();
    return nullptr;
}

pthread_t _daemon = 0;

scheduler::scheduler(){
    cpu_init();
    
    this->p_count = this->p_count == 0 ? CPUS : this->p_count;
    get_coprimes(coprime_arr, this->p_count);
    if(this->p_count == 0) return;

    p_count = 1;
    for(int j=0;j<p_count;j++){ // 需要先初始化，否则 all_processor 会并发访问
        this->local_tasks.push_back(new atomic_queue);
    }
}

void scheduler::init(){
        // TODO 注意执行时机
    pthread_create(&_daemon, NULL, background, nullptr);
    pthread_detach(_daemon); // 守护线程

    back_cond.wait(); //等待守护线程创建完成

    for(int i=0;i<p_count;i++){
        atomic_queue* p = this->local_tasks[i];
        machine* m = new machine(p);
        this->m_run.push(m);  //加入全局队列；
    }
}

bool scheduler::join(atomic_queue* p, machine* m){ // 未激活的p 绑定m

    if (m == nullptr ) {
        sched->m_idle.pop(m);
        if (m != nullptr) {
            m->p = p;
            m->lock.notify(); // 唤醒一个m
        } else {
            m = new machine(p); // 新建一个m
        }
    }

    m->state = M_STATE::start;
    sched->m_run.push(m);

    return true;
}

scheduler* sched = nullptr;
mutex sched_mtx;

aco_t* _co = nullptr;

void go(function<void()> func, task* co) {
    
    if (sched == nullptr) { // 二次筛选，减少锁数量
        sched_mtx.lock();
            if (sched == nullptr) {
                aco_thread_init(NULL);
                _co = aco_create(NULL,NULL, 0, NULL, NULL);

                sched = new scheduler();
                sched->init();
            }
        sched_mtx.unlock();
    }

    if (co == nullptr) {
        co = new task(_co, func);
    }

    if (local_m != nullptr) {

        if (local_m->p !=nullptr && local_m->p->len() < max_queue_len) { //添加到本地队列
            local_m->p->push(co);
            return;
        }
    }


    // 唤醒一个空闲的p
    atomic_queue* p = nullptr;
    sched->p_idle.pop(p);

    if (p != nullptr) {
        p->push(co);
        sched->join(p);

        return;
    }
    sched->global_tasks.push(co); //放入全局task
}

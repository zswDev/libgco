#include <iostream>

#include "concurrentqueue.h"
#include <thread>
#include <iostream>

#include "machine.h"
#include "scheduler.h"

using namespace std;



bool machine::work_stealing(int offset){
    atomic_queue* other_p = sched->local_tasks[offset];
    if(other_p == nullptr) return false;

    int work_len = other_p->len();
    if (work_len == 0) return false;

    int get_len = work_len - work_len/2;
    
    task* wait_co[get_len];

    int idx = 0;
    for(int i=0;i<get_len;i++) {
        task* co = nullptr;
        other_p->pop(co);
        if (co != nullptr) {
            ++idx;
            this->p->push(co);
        }
    }
    
    return idx != 0; // 偷取成功, 只偷一次, ret==0代表没成功
}

task* machine::get_task(){
    task* co = nullptr;
Start:    
    while (this->p->pop(co)) { // 拿到了
        if (co != nullptr) return co;
    }

    if(sched->global_tasks.len() != 0) { // 从全局队列获取
        task* wait_co[max_queue_len] = {};      //没有初始化内存会报错
        
        int idx = 0;
        for(int i=0;i<max_queue_len; i++) {
            task* co = nullptr;
            sched->global_tasks.pop(co);
            if (co != nullptr) {
                this->p->push(co);
                ++idx;
            }
        }
        if (idx != 0) goto Start; // 去执行
    }
   
    if(sched->p_idle.len() ==  sched->p_count - 1) {  //  全都sleep中 则sleep
        goto Stop;
    }
    
    if (thief_work_len >= sched->p_count) { // 寻找者太多 则sleep
        goto Stop;
    }

    ++thief_work_len;
    for(int i=0; i< 4; i++) { // TODO 偷取非空闲的p

        if (sched->p_count == 2) {
            int offset = get_random(sched->p_count);
            bool ret = work_stealing(offset);
            if(!ret) {
                offset = offset == 1 ? 0 : 1;
                ret = work_stealing(offset); 
            } 

            if(ret) {
                --thief_work_len;
                goto Start; // 偷到了
            }
        } else {
            int offset = get_random(sched->p_count);
            int coprime = get_random(sched->coprime_arr.size());
            coprime = sched->coprime_arr[coprime];

            bool ret = false;
            for(int i=0;i<sched->p_count;i++){
                
                ret = work_stealing(offset);
                if (ret) break;
                                
                offset += coprime;
                offset = offset % sched->p_count;
            }
            
            if(ret) {
                int _len = this->p->len();
                cout<<"get co ok:"<<_len<<endl;
                --thief_work_len;
                goto Start;// 偷到了
            }
        }
    
    }
    --thief_work_len;

Stop:
    if(this->p->len() == 0) {
        //加入空闲队列
        sched->p_idle.push(this->p);
        this->p = nullptr;
    } else {
         goto Start;
    }
    return nullptr;
}


thread_local machine* local_m = nullptr;

thread_local aco_t* local_main_co = nullptr;
thread_local int i=0;

void* m_loop(void* args){
    machine* _m =(machine*) args;
    
    if (_m == nullptr) return nullptr;
    local_m = _m;

    if (local_main_co == nullptr) {
        aco_thread_init(NULL);
        local_main_co = aco_create(NULL,NULL, 0, NULL, NULL);
    }
    local_m->main_co = local_main_co;

    while(true) {

Start:
        if (local_m->co == nullptr) { //没有可运行的co

            if(local_m->p == nullptr) { 

                if(sched->p_idle.pop(local_m->p)) { //从全局空闲队列获取p

                    if(!sched->join(local_m->p, local_m)){ //获取到了尝试邦定
                        local_m->p = nullptr;
                    }
                }         
            }

            if (local_m->p == nullptr) {
                goto Stop;
            }

            local_m->co = local_m->get_task(); //从绑定的p 获取co
            if (local_m->co == nullptr) {
                goto Stop;
            }
        }

        //cout<<"co:"<<local_m->co<<endl;

        try{
            local_m->co->run(local_m->main_co);
        }catch(const char* err){
            cout<<"err: "<<err<<endl;
            throw err;
        }

        if (local_m->state == M_STATE::block) {  //系统调用完了恢复该co
            local_m->state = M_STATE::start;

            local_m->p = nullptr;
            sched->p_idle.pop(local_m->p); //获取空闲p
            
            if(local_m->p == nullptr ||
            local_m->p->len() >= max_queue_len) {
                
                if (local_m->co != nullptr) {
                    sched->global_tasks.push(local_m->co);  // 放入全局队列 
                }
                local_m->co = nullptr;
                goto Stop;
            } 

            // 放入本地队列前需要判断状态?
        }
        
        //cout<<"state"<<local_m->co->state<<endl;     

        if (local_m->co->state == TASK::freezed) { // 进入系统调用 或锁定了, hook系统调用
            //cout<<"syscall"<<endl;

            sched->join(local_m->p); //p选择一个新的m
            
            local_m->state = M_STATE::block;
            local_m->p = nullptr;

            goto Start; // 继续系统调用
        } else if (local_m->co->state == TASK::actived){
            local_m->p->push(local_m->co); // 添加回队尾
        }
        //  else if(local_m->co->state == TASK::deathed) {
        //     cout<<"co death"<<endl;
        // } else if (local_m->co->state == TASK::polling) {
        //   cout<<"io poll"<<endl;
        //}

        local_m->co = nullptr;

        goto Start;
    Stop:
        cout<<"stop："<<local_m<<endl;

        if (local_m->p != nullptr) {
            sched->p_idle.push(local_m->p); // 加入到全局空闲队列；
        }
        local_m->p = nullptr; //解绑p

        local_m->state = M_STATE::wait;
        sched->m_run.del(local_m);  //从运行队列删除
        sched->m_idle.push(local_m);  // 加入到全局空闲队列；

        local_m->lock.wait();     // 等待唤醒
        sched->m_run.push(local_m); //唤醒后加入运行队列
    }
}

machine::machine(atomic_queue* p){
    this->p = p;

    pthread_create(&this->thid, NULL, m_loop, this);
}

pthread_t machine::get_tid(){
    return this->thid;
}

#include "task.h"

void co_box(){
    func_param* run_ptr = (func_param*)aco_get_arg();
    func_param  run = *run_ptr; // 获取实际函数

    aco_yield();
    
    run();

    aco_exit();
};

// 随机一个main_co
task::task(aco_t* main_co,func_param func){
    this->params = (void*)&func;
    this->sstk = aco_share_stack_new(0);
    aco_t* co_ptr = aco_create(main_co, sstk, 0, co_box, this->params);
    this->co = move(*co_ptr);

    aco_resume(&(this->co));
}

void task::run(aco_t* main_co){ // 线程的main_co
    if(this->co.is_end) return;
        
    this->co.main_co = main_co; // 这个是每个线程都存在
    aco_resume(&(this->co));

    if(this->co.is_end) {
        this->state = TASK::deathed;
    }
}

#ifndef __task_h__
#define __task_h__

#include<atomic>
#include<functional>
#include "aco.h"

using namespace std;

typedef function<void(void)> func_param;

extern void co_box();

enum TASK{
    actived,
    freezed,
    polling,
    deathed
};

struct task{
private:
    aco_share_stack_t* sstk;
    void* params;
public:
    aco_t co;
    atomic<int> state = {TASK::actived};
    task(aco_t* main_co,func_param func);
    void run(aco_t* main_co);
};

#endif

#include "co_io.h"
#include "task.h"
#include "uv_fs.h"
#include "uv_timer.h"
#include "scheduler.h"

task* get_async_co(){
    aco_t* self = aco_get_co();
    
    task* co = container_of(self, task, co);
    if(co != nullptr) {
        co->state = TASK::polling;
    }

    return co;
}

task* get_sync_co(){
    task* co = get_async_co();
    if (co != nullptr) {
        co->state = TASK::freezed;
    } 
    
    return co; 
}

void _syscall(){
    task* co = get_sync_co();
    if(co == nullptr) return;

    aco_yield(); // 先暂停，再继续系统调用

    int t = get_random(10000) + 1000*2;
    usleep(t);
}

void co_sleep(int ms){
    task* co = get_async_co();
    if(co == nullptr) return;

    setTimeout([&](){
        cout<<"a"<<endl;
        go(NULL, co);
    },ms);

    aco_yield();
}

int co_open(const char* path, int flags,int mode){
    task* co = get_async_co();
    if(co == nullptr) return -1;

    int ret_fd = 0;
    fs::open(path, flags, mode, [&](int fd){
        ret_fd = fd;
         go(NULL, co);
    });

    aco_yield();
    return ret_fd;
}

ssize_t co_read(int fd, void* buf, size_t count){
    task* co = get_async_co();
    if(co == nullptr) return -1;

    int ret_count = 0;
    fs::read(fd, buf, count, [&](int _count){
        ret_count = _count;
         go(NULL, co);
    });

    aco_yield();
    return ret_count;
}

ssize_t co_write(int fd, const void* data, size_t count){
    task* co = get_async_co();
    if(co == nullptr) return -1;

    int ret_count = 0;
    fs::write(fd, data, count, [&](int _count){
        ret_count = _count;
         go(NULL, co);
    });

    aco_yield();
    return ret_count;
}

void co_close(int fd){
    fs::close(fd);
}

void co_loop(){
    uv_loop_t* loop = uv_default_loop();
    uv_run(loop, UV_RUN_DEFAULT);

    uv_loop_close(loop);
    free(loop);
}
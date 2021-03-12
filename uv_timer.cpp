#include<iostream>
#include <uv.h>
#include <functional>

#include "uv_timer.h"

using namespace std;

timer_queue time_q;

void time_cb(uv_timer_t* timer_req){
    timer* t = container_of(timer_req, timer, req);
    
    if(t != nullptr) {
        t->cb();
    }
}

void main_handler(uv_async_t* handle){
    timer* t = (timer*) handle->data;

    if ( t!= nullptr) {
        //cout<<t->repeat<<","<<t->ms<<endl;
        uv_loop_t* loop = uv_default_loop();
        uv_timer_init(loop, &t->req); // 初始化定时器
        uv_timer_start(&t->req, time_cb, t->repeat, t->ms);
    }
    uv_close((uv_handle_t*)handle, NULL);
}

void timer::add(function<void()>_cb,int timeout, int _repeat) {
    this->cb = _cb;
    this->ms = timeout;
    this->repeat = _repeat;

    uv_async_init(uv_default_loop(), &async, main_handler); // 初始化通信器
    async.data = this;
    uv_async_send(&this->async);
}

void timer::main_add(function<void()>_cb,int timeout,int _repeat) {
    this->cb = _cb;
    
    uv_timer_init(uv_default_loop(), &this->req);
    uv_timer_start(&this->req, time_cb, timeout, _repeat);
}

void timer::close(){
    uv_timer_stop(&req);
}



timer* setTimeout(function<void()> task, int ms){
    timer* t = new timer;
    t->add([=](){
        task();
        t->close();
    },0,ms);

    return t;
}
void clearTimeout(timer* t){
    t->close();
    delete t;
}

timer* setInterval(function<void()> task, int ms){
    timer* t = new timer;
    t->add(task, ms, ms);

    return t;
}
void clearInterval(timer* t){
    t->close();
    delete t;
}

/*
int main(){
    // setInterval([](){
    //     cout<<"setInterval"<<endl;
    // }, 2000);

    // timer* t = new timer([&](){
    //     cout<<"111111"<<endl;
    //     t->close();
    // }, 1000);
    
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}*/
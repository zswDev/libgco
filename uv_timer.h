#ifndef __uv_timer_h__
#define __uv_timer_h__

#include<iostream>
#include <uv.h>
#include <functional>
#include "concurrentqueue.h"

using namespace std;

#define  container_of(ptr, type, member) ({    \
     const typeof( ((type *)0)->member ) *__mptr = (ptr); \
     (type *)( (char *)__mptr -  ((size_t) &((type *)0)->member) );})

struct timer;

typedef moodycamel::ConcurrentQueue<timer*> timer_queue; 

extern timer_queue time_q;

struct timer{
    uv_timer_t req;
    int ms=0;
    int repeat=0;
    function<void()> cb;

    uv_async_t async;

    void add(function<void()> _cb,int timeout,int _repeat);
    void main_add(function<void()> _cb,int timeout,int _repeat);
    void close();

    friend void time_cb(uv_timer_t*);
};

extern void main_handler(uv_async_t*);// TOOD 注意在loop线程执行

extern timer* setTimeout(function<void()> cb, int ms);
extern void clearTimeout(timer* t);
extern timer* setInterval(function<void()> cb, int ms);
extern void clearInterval(timer* t);

#endif
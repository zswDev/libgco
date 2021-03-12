#ifndef __uv_fs_h__
#define __uv_fs_h__


#include <iostream>
#include <functional>
#include <map>
#include <string.h>
#include <memory.h>
#include <stdlib.h> 
#include <uv.h>
#include <sys/time.h>

// #include "promise.h"
// #include "corotine.h"

//#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define  container_of(ptr, type, member) ({    \
     const typeof( ((type *)0)->member ) *__mptr = (ptr); \
     (type *)( (char *)__mptr -  ((size_t) &((type *)0)->member) );})

using namespace std;

typedef function<void(ssize_t size)> fs_callback;

extern void on_open(uv_fs_t* req);
extern void on_read(uv_fs_t* req);
extern void on_write(uv_fs_t* req);

struct File{
    static map<int, File*> cache_fd;

    int fd = 0;
    uv_fs_t fs_req;

    fs_callback cb;
    size_t count = 0;
    int chunk = 0;

    //const void* buf;
    uv_buf_t data;

    void _error(const char* title, ssize_t result);

    void _open(const char* pathname,int flags,int mode,fs_callback cb, uv_fs_t* req);
    void _read(int fd, void* buf, size_t count, fs_callback cb, uv_fs_t* req);
    void _write(int fd, const void* data, size_t count, fs_callback cb, uv_fs_t* req);
    void _close();
    friend void on_open(uv_fs_t* req);
    friend void on_read(uv_fs_t* req);
    friend void on_write(uv_fs_t* req);
};

class fs{
public:
    static void open(const char* path, int flags,int mode, fs_callback cb);
    static void read(int fd, void* buf, size_t count, fs_callback cb);
    static void write(int fd, const void* data, size_t count, fs_callback cb);
    static void close(int fd);
};

#endif
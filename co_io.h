#ifndef __co_io_h__
#define __co_io_h__

#include "task.h"

extern task* get_async_co();

extern task* get_sync_co();

extern void _syscall();

extern void co_sleep(int ms);

extern int co_open(const char* path, int flags,int mode);

extern ssize_t co_read(int fd, void* buf, size_t count);

extern ssize_t co_write(int fd, const void* data, size_t count);

extern void co_close(int fd);

extern void co_loop();

#endif
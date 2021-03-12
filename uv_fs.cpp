#include <iostream>
#include <functional>
#include <map>
#include <vector>
#include <string.h>
#include <memory.h>
#include <stdlib.h> 
#include <uv.h>
#include <sys/time.h>

// #include "promise.h"
// #include "corotine.h"
#include "uv_fs.h"

using namespace std;

map<int, File*> File::cache_fd = {};

void File::_error(const char* title, ssize_t result){
    string err = title + (string)uv_strerror(result);
    cout<<err<<endl;
    throw "err";
}

void File::_open(const char* _path,int flags,int mode, fs_callback cb, uv_fs_t* req = nullptr){
    if(req == nullptr) {
        this->cb = cb;

        uv_loop_t* loop = uv_default_loop();
        uv_fs_open(loop, &this->fs_req, _path, flags, mode, on_open); //先不关闭
        return;
    }

    // 友元函数调用
    if(req->result >= 0) {
        cache_fd[req->result] = this; // 用于其他函数进行关闭

        this->cb(req->result); // 传入fd; 执行完回调后关闭fd, 不能关闭，因为回调不确定
    } else {
        this->_error("open err:", req->result);
        this->_close();
    }  
    
}

void File::_read(int fd, void* buf, size_t count, fs_callback cb, uv_fs_t* req = nullptr) {
    if(req == nullptr) {
        this->cb = cb;
        this->fd = fd;
        //this->buf = buf;

        this->data = uv_buf_init((char*)buf, count);

        uv_loop_t* loop = uv_default_loop();
        uv_fs_read(loop, &this->fs_req, fd, &this->data,1,-1, on_read);

        return;
    }

    // 友元函数调用
    if (req->result < 0) {
        this->_error("read error:", req->result);
        this->_close();
    } else if(req->result > 0){ // 继续读取

        chunk += req->result; //记录读取长度
        
        uv_loop_t* loop = uv_default_loop();
        uv_fs_read(loop, &this->fs_req, this->fd, &this->data,1,-1, on_read);
    } else if(req->result == 0) { // 读完了
        
        this->cb(this->chunk);
        this->_close();        
    }
    
}


void File::_write(int fd, const void* _data, size_t count, fs_callback cb, uv_fs_t* req = nullptr){
    
    if(req == nullptr) {
        this->cb = cb;
        this->fd = fd;
        this->count = count;        
        this->data = uv_buf_init((char*)_data, count);

        uv_loop_t* loop = uv_default_loop();
        uv_fs_write(loop, &this->fs_req, fd, &this->data, 1, -1, on_write);
        return;
    }
    // 友元函数调用
    if (req->result < 0) {
        this->_error("write err", req->result);
        this->_close();
        return;
    } 

    this->chunk += req->result;

    if(this->chunk == this->count) { // 写完了
        this->cb(this->chunk);
        //this->_close();
    } else {

        // 没写完，写入下一段数据
        uv_loop_t* loop = uv_default_loop();
        uv_fs_write(loop, &this->fs_req, this->fd, &this->data, 1, -1, on_write);
    }
    
}

void File::_close(){
    uv_fs_t close_req;
    uv_fs_close(uv_default_loop(), &close_req, fs_req.result, NULL);
    uv_fs_req_cleanup(&fs_req);
}

void on_open(uv_fs_t* req) {
    File* _fs = container_of(req, File, fs_req);
    _fs->_open("",0,0,NULL,req);
}

void on_read(uv_fs_t* req){
    File* _fs = container_of(req, File, fs_req);
    _fs->_read(0,0,0,NULL, req);
}

void on_write(uv_fs_t* req){
    File* _fs = container_of(req, File, fs_req);
    _fs->_write(0,0,0,NULL, req);
}


void fs::open(const char* path, int flags,int mode, fs_callback cb){
    File* f = new File;
    f->_open(path, flags, mode, cb);
}

void fs::read(int fd, void* buf, size_t count, fs_callback cb){
    File* f = new File;
    f->_read(fd, buf, count, cb);
}

void fs::write(int fd, const void* data, size_t count,  fs_callback cb){
    File* f = new File;
    f->_write(fd, data, count, cb);
}

void fs::close(int fd){
    File* fs = File::cache_fd[fd];
    if(fs != nullptr) {
        int open_fd = fs->fd;
        File* open_fs = File::cache_fd[open_fd];
        if(open_fs != nullptr) {
            open_fs->_close();
            File::cache_fd.erase(open_fd);
        }
        fs->_close();

        File::cache_fd.erase(fd);
    }
}

// long now(){
//     timeval tv;
//     gettimeofday(&tv, NULL);

//     return tv.tv_sec * 1000 + tv.tv_usec/1000;
// }

/*
//uv_fs_t open_fs;
int main(){
    char* data = "abcdef";
    
    uv_loop_t* loop = uv_default_loop();    
    
    //uv_fs_open(loop, &open_fs, "./p1.js",  O_RDWR | O_APPEND, S_IRUSR | S_IWGRP, on_open);

    fs::open("./p1.js", O_WRONLY, 0, [&](int fd){

      //  printf("fd: %i \n", fd);
        // char* buf =(char*)malloc(sizeof(char)*5); // 注意回调函数会 关闭堆栈
        // fs::read(fd, buf, 5, [=](int count){
        //     cout<<"c"<<count<<endl;
        //     printf("%s", buf);
        //     //cout<<buf<<endl;
        // });
        // char* data="77777"; // 注意堆栈回收
        // // buf = "abcde";
        fs::write(fd, data, strlen(data), [=](int count){
            cout<<count<<endl;

            fs::close(fd);
        });
       
    });
    
   // auto pf = []()->promise*{
        //return new promise([=](cb_func resolve, cb_func reject){
            // fs::read("./p.js", [=](const char* err, char* data){
            //     if (err != nullptr) {
            //         reject((char*)err);
            //     } else {
            //         resolve(data);
            //     }
            // });
        // char* data = "123456";
        // fs::write("./p.js", data, [=](const char* err, char* data){
        //     if (err != nullptr) {
        //         cout<<err<<endl;
        //         reject((char*)err);
        //     } else {
        //         resolve(data);
        //     }
        // });
    //    });
    //};
    //  async ([=](void* args){
    //      //cout<<1<<endl;
    //      void* data = await pf();
    //      cout<<"over"<<endl;
    //      cout<<(char*)data<<endl;
    //      data = await pf();
    //      cout<<"over"<<endl;
    //      cout<<(char*)data<<endl;
    //  });


   // char* data = "33333";
    // fs::write("./p.js", data, [=](const char* err, char* data){
    //     cout<<"over"<<endl;
    // });

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

}
*/

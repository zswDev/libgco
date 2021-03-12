
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include <iostream>
#include <thread>
#include <functional>
#include <atomic>

#include <uv.h>

#include "uv_net.h"

using namespace std;

void _err(int r) {
    cout<<"err:"<<uv_strerror(r)<<endl;
    exit(0);
}


uv_tcp_t server;

// ///分配空间存储接受的数据
void _alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
 buf->base = (char*)malloc(suggested_size);
 buf->len = suggested_size;
}

/// 将从socket套接字读取的数据放⼊request(req) 然后在写(buf->base nread 个字节)后 调⽤回调函数检查状态 释放req占⽤的内存
/// 这⾥要注意 正确读取的时候req由回调函数处理
/// ⽽EOF/其他错误发⽣的时候 需要关闭套接字 并释放buf->base所占据的内存
/// EOF代表套接字已经被关闭
/// 因为此时没有回调函数
/// 异步回调很容易出错
typedef struct {
 uv_write_t req;
 uv_buf_t buf;
} write_req_t;

/// 释放资源的回调函数
void free_write_req(uv_write_t* req) {
 write_req_t* wr = (write_req_t*)req;
 free(wr->buf.base);
 free(wr);
}

/// 写完成后调⽤的函数
/// 释放资源
void echo_write(uv_write_t* req, int status) {
    if (status) {
    fprintf(stderr, "Write error %s\n", uv_strerror(status));
    }
    free_write_req(req);
}

void echo_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
    if (nread > 0) {
        cout<<"server_get:"<<buf->base<<endl;

        write_req_t* req = (write_req_t*)malloc(sizeof(write_req_t));
        req->buf = uv_buf_init(buf->base, nread);
        uv_write((uv_write_t*)req, client, &req->buf, 1, echo_write);
        return;
    }
    
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        }

        uv_close((uv_handle_t*)client, NULL);
    }
    free(buf->base);
}

void on_new_connection(uv_stream_t* server, int status) {
    if (status < 0) {
        _err(status);
    }

    uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_loop_t* loop = uv_default_loop();
    uv_tcp_init(loop, client);

    cout<<"start"<<endl;
    if (uv_accept(server, (uv_stream_t*) client) == 0) {
        uv_read_start((uv_stream_t*)client, _alloc_buffer, echo_read);
    } else {
        uv_close((uv_handle_t*)client, NULL);
    }
}

void service(){
    uv_loop_t* loop = uv_default_loop();

    uv_tcp_init(loop, &server);
    sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", 8001, &addr);

    uv_tcp_bind(&server, (const sockaddr*)&addr, 0);

    int r = uv_listen((uv_stream_t*)&server, 128, on_new_connection);
    if (r) {
        _err(r);
       
    }
    uv_run(loop, UV_RUN_DEFAULT);
}

namespace net{

server* createServer(client_cb cb) {
    return new server(cb);
}

server::server(client_cb cb) {
    this->clientcb = cb;
}

void server::connect(uv_stream_t* conn, int status) {
    if (status < 0) {
        _err(status);
    }
    if (this->done != NULL) {
        this->done();
    }

    //uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    client* c = (client*)malloc(sizeof(client));
    
    uv_loop_t* loop = uv_default_loop();
    uv_tcp_init(loop, &c->sockfd);

    //cout<<"start"<<endl;
    if (uv_accept(conn, (uv_stream_t*) &c->sockfd) == 0) {
        
        c->conn.handle =  (uv_stream_t*) &c->sockfd;
        this->clientcb(c);

        //this->clientcb();
       // uv_read_start((uv_stream_t*)client, _alloc_buffer, echo_read);
    } else {
        uv_close((uv_handle_t*)&c->sockfd, NULL);
    }
}

void server::listen(int port, function<void()> done){
    
    uv_loop_t* loop = uv_default_loop();
    uv_tcp_init(loop, &sockfd);

    uv_ip4_addr("0.0.0.0", 8001, &addr);
    uv_tcp_bind(&sockfd, (const sockaddr*)&addr, 0);

    this->done = done;

    int r = uv_listen((uv_stream_t*)&sockfd, 128, _sconnect);

    if (r) {
        printf("failed to listen on the address, err: %s\n", uv_strerror(r));
        err(r);
    }
}
    


void err(int r) {
    cout<<"err:"<<uv_strerror(r)<<endl;
    exit(0);
}

///分配空间存储接受的数据
void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = (char*)malloc(suggested_size);
    buf->len = suggested_size;
}

void client::connect(uv_connect_t* conn, int status){
    if (conn == nullptr) {
        uv_loop_t* loop = uv_default_loop();

        uv_tcp_init(loop, &sockfd);

        uv_ip4_addr(ip, port, &dest);

        uv_tcp_connect(&this->conn, &sockfd, (const sockaddr*)&dest, _cconnect);
    } else {


        if (status < 0) {
            err(status);
        }
        this->conncb(this);
    }
}


client::client(const char* ip, int port, client_cb cb){
    this->ip = ip;
    this->port = port;
    this->conncb = cb;

    this->connect(nullptr, 0);
}

client* connect(const char* ip, int port, client_cb cb){
    client* c = new client(ip, port, cb);
    return c;
}

void client::read(read_cb readcb,uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf){
    if(stream == nullptr) { // 第一次读
        this->readcb = readcb;

        conn.handle->_args = this; //魔改了 uv_stream_t
  
        uv_read_start(conn.handle, alloc_buffer, _read);
    } else {

        if (nread < 0) {
            fprintf(stderr, "failed to read from server, err: %s\n", uv_strerror(nread));
            free(buf->base);
            exit(1);
            return;
        } else if (nread > 0) {
            //buf->base[nread] = '\0';
            //printf("recv %s\n", buf->base);
           
            this->readcb(buf->base, nread);

            //uv_shutdown_t *sd = (uv_shutdown_t*)malloc(sizeof(uv_shutdown_t));
            //uv_shutdown(sd, stream, on_shutdown);
        } else if (nread == 0) { // 读取完了

            cout<<"_over1"<<endl;
            free(buf->base);

            // TODO 这里关闭？
        }
    }
}

void client::write(char* data, ssize_t count,  uv_write_t* req, int status){
    if (req == nullptr) {
        
        writefd* _req = new writefd; //每次写都要一个新的writefd
        _req->conn = this;
        _req->buf = uv_buf_init(data, count);

        int r = uv_write(&_req->fd, conn.handle, &_req->buf,1, _write);

        if (r != 0) {
            printf("failed to send hello to server, err: %s\n", uv_strerror(r));
        }

    } else {

        if (status < 0) {
            fprintf(stderr, "failed to write hello to server, err: %s\n", uv_strerror(status));
        } else {
            return;
        }

        fprintf(stderr, "uv_write error: %s\n", uv_strerror(status));
        if (status == UV_ECANCELED)
            return;
        assert(status == UV_EPIPE);
        
        uv_close((uv_handle_t *)req->handle, NULL);
        free(req);
    }
}
void client::send(char* data, ssize_t count) {
    write(data, count, nullptr, 0);
}

void client::recv(read_cb readcb) {
    read(readcb, nullptr, 0, nullptr);
}

void on_close(uv_handle_t* handle) {
    free(handle);
}
static void on_shutdown(uv_shutdown_t *sd, int status) {
    if (status < 0) {
        fprintf(stderr, "failed to shutdown connection, err: %s\n", uv_strerror(status));
        return;
    } else {
        uv_close((uv_handle_t *)sd->handle, on_close);
    }
    free(sd);
}

void client::close(){
    
    //uv_shutdown_t* sd = (uv_shutdown_t*)malloc(sizeof(uv_shutdown_t));
    //uv_shutdown(sd, conn.handle, on_shutdown);
    
    //uv_close((uv_handle_t*)&sockfd, NULL);
    //uv_close((uv_handle_t*)conn.handle, NULL);
}


}

int main(){
 
    thread th1([]{
         sleep(2);
        net::client* c1 = net::connect("0.0.0.0", 8001, [=](net::client* c1){  
            cout<<"conn"<<endl;

            int count = 2;
            char* data = (char*)malloc(sizeof(char)*count);
            for(int i=0;i<count;i++){
                    data[i] = 'a';
            }
            data[count] = '\0';

            cout<<count<<endl;

            for(int i=0;i<1;i++) {
                //sleep(1);
                c1->send(data, strlen(data));
            }

            atomic<int> idx = {0};
            c1->recv([&](char* data, ssize_t count){
                cout<<"client_get:"<<data<<endl;
                //cout<<count<<endl;
                ++idx;

                c1->close();
                cout<<"； read_over："<<idx<<endl;
            });
            
        });

        uv_run(uv_default_loop(), UV_RUN_DEFAULT);
        
        //cout<<"over"<<endl;
     });


    net::server* s = net::createServer([](net::client* c){
        cout<<"new client"<<endl;
        c->recv([&](char* data, ssize_t len) {
            cout<<"server_get:"<<data<<endl;

            c->send(data, len);
        });
    });
    s->listen(8001);

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    sleep(3);

}
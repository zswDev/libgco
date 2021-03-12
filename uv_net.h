#ifndef __uv_net_h__
#define __uv_net_h__

#include <functional>

#include <uv.h>

#define  container_of(ptr, type, member) ({    \
     const typeof( ((type *)0)->member ) *__mptr = (const typeof( ((type *)0)->member ) *) (ptr); \
     (type *)( (char *)__mptr -  ((size_t) &((type *)0)->member) );})

using namespace std;


namespace net {
    struct client;
    typedef function<void(client*)> client_cb;

    struct server{
    private:
        uv_tcp_t sockfd;
        sockaddr_in addr;
        const char* ip = "";
        int port = 0;
        client_cb clientcb;
        function<void()> done;

        void connect(uv_stream_t* conn, int status);
    public:
        server(client_cb cb);
        void listen(int, function<void()> = NULL);

        friend void _sconnect(uv_stream_t* conn, int status);
    };

    void _sconnect(uv_stream_t* conn, int status) {
        server* s = container_of(conn, server, sockfd);
        s->connect(conn, status);
    }   

    extern server* createServer(client_cb cb);

    typedef function<void(char*, ssize_t)> read_cb;

    void err(int r);
    void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);

    struct writefd {
        client* conn;
        uv_write_t fd;
        uv_buf_t buf;
    };

    struct client{
    private:
        const char* ip = "";
        int port = 0;
        client_cb conncb;
        read_cb readcb;

        sockaddr_in dest;
       
       
       
        uv_buf_t cache;

        void connect(uv_connect_t* conn, int status);
        void read(read_cb readcb, uv_stream_t *stream = nullptr, ssize_t nread = 0, const uv_buf_t *buf = nullptr);
        void write(char* data, ssize_t count, uv_write_t* req = nullptr, int status = 0);

    public:
        uv_connect_t conn;
        uv_tcp_t sockfd;

        client(const char* ip, int port, client_cb cb);
   
        void send(char* data, ssize_t count);
        void recv(read_cb readcb);
        void close();

        friend void _cconnect(uv_connect_t* conn, int status);
        friend void _write(uv_write_t* req, int status);
        friend void _read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
    };


    void _cconnect(uv_connect_t* conn, int status){
        client* c = container_of(conn, client, conn);
        c->connect(conn, status);
    }

    void _read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
        client* c = (client*)stream->_args;
        c->read(NULL, stream, nread, buf);
    }

    void _write(uv_write_t* req, int status) {
        writefd* w = container_of(req, writefd, fd);
        w->conn->write(nullptr, 0, req, status);

        //free(w->buf.base);
        free(w); // ???每次写完都回收了
    }

    client* connect(const char* ip, int port, client_cb cb);
}

#endif
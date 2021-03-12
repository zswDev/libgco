#include <iostream>

#include <unistd.h>
#include <sys/time.h>

#include "machine.h"
#include "scheduler.h"
#include "co_io.h"


using namespace std;

class T{
public:
    int i=123;
    T();
    ~T();
};
T::T(){
    cout<<"new"<<endl;
}
T::~T(){
    cout<<"del"<<endl;
}

long now(){
    timeval tv;
    gettimeofday(&tv, NULL);

    return tv.tv_sec * 1000 + tv.tv_usec/1000;
}

atomic<int> over = {0};
int main(){

    atomic<long> start = {now()};

    for(int i=0;i<1024;i++){
        go([&](){
            
            //cout<<"user start"<<endl;
            co_sleep(1000);
            //aco_yield();
            //_syscall();
            //cout<<"----------------"<<endl;
            //_syscall();
            ++over;
            //atomic<long> end = {now()};
            //cout<<"time: "<<end-start<<endl;
            cout<<"over:"<<over<<endl;
        });
    }
    
    sleep(5);
    //co_loop();//主线程开启co loop, 否则会报错
    cout<<"----------------------"<<endl;
}
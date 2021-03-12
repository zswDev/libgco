#ifndef __channel_h__
#define __channel_h__

#include <iostream>
#include <mutex>
#include <condition_variable>


using namespace std;

template<typename DATA>
struct _node{
    DATA data;
    _node* next;
    _node(DATA d): data(d) {};
};


template<typename DATA>
class channel {
private:
    condition_variable cond;
    mutex mtx;
    
    _node<DATA>* head = nullptr;
    _node<DATA>* tail = nullptr;
    
    int idx = 0;
    int max_len = 0;
public:
    channel(int max_length = 8){
        max_len = max_length;
    }

    void operator<<(DATA data) {
        _node<DATA>* n1 = new _node<DATA>(data);

        unique_lock<mutex> lk(mtx); //会自动解锁 

        cond.wait(lk, [&]{return idx < max_len || idx == 0;});
        
        //cout<<"write"<<endl;

        //cout<<"stop read"<<endl;

        if (head == nullptr) {
            head = n1;
        }

        if (tail != nullptr) {
            tail->next = n1;
        }
        tail = n1;
        ++idx;


        cond.notify_one(); //唤醒一个

        //cout<<"write over"<<endl;
    }

    void operator>>(DATA& result){
        unique_lock<mutex> lk(mtx); //自动解锁

        cond.wait(lk, [&]{return idx > 0;});

        //cout<<"stop write"<<endl;

        if(head != nullptr) {

            result = head->data;
            head = head->next;
            
            --idx;
        }

        cond.notify_one();//唤醒一个
        
        //cout<<"read over"<<endl;
    }
};

#endif
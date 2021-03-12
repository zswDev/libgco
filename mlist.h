#ifndef __mlist_h__
#define __mlist_h__


#include<functional>
#include<mutex>

using namespace std;

template<class T>
struct _node{
    T m;
    _node* next;
    _node(T _m):m(_m) {};
};

template<class T>
class mlist{
    _node<T>* head = nullptr;
    _node<T>* tail = nullptr;
    mutex mtx;

public:

    void push(T);

    bool pop(T&);

    void del(T);

    int len();

    void filter(function<bool(T)>);
};


template<class T>
void mlist<T>::push(T m){
    mtx.lock();

    _node<T>* n1 = new _node<T>(m);

    if(this->head == nullptr) {
        this->head = n1;
    }

    if (this->tail != nullptr) {
        this->tail->next = n1;
    }

    this->tail = n1;

    mtx.unlock();
}

template<class T>
bool mlist<T>::pop(T& m){
    
    if (head != nullptr) {
        m = head->m;
        head = head->next;

        return true;
    }
    return false;
}

template<class T>
void mlist<T>::del(T m){
    this->mtx.lock();

    _node<T>* last = nullptr;
    _node<T>* that = this->head;

    while(that != nullptr) {

        if (that->m == m) {
            if(last != nullptr) { // 链接上下节点
                last->next = that->next;
            }

            that = that->next;

            continue;  // 进行下一个循环
        }
        last = that;
        that = that->next;
    }

    this->mtx.unlock();
}

template<class T>
int mlist<T>::len(){
    int length = 0;
    this->mtx.lock();

    
    _node<T>* that = this->head;

    while(that != nullptr) {
        ++length;
        that = that->next;
    }

    this->mtx.unlock();

    return length;
}

template<class T>
void mlist<T>::filter(function<bool(T)> user_cb){ //检测线程是否退出

    this->mtx.lock();
    _node<T>* last = nullptr;
    _node<T>* that = this->head;
    while(that != nullptr) {

        T m = that->m;
        bool ret = user_cb(m);
        if(ret) {

            if(last != nullptr) { // 链接上下节点
                last->next = that->next;
            }

            that = that->next;

            continue;
        }

        last = that;
        that = that->next;
    }
    this->mtx.unlock();
}

#endif
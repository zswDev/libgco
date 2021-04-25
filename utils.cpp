#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#include "utils.h"

int CPUS = 0;

cpu_set_t* CPU_masks = 0;

void cpu_init() {
    CPUS = sysconf(_SC_NPROCESSORS_CONF);
    cpu_set_t* CPU_masks = (cpu_set_t*)malloc(sizeof(cpu_set_t)* CPUS);
};

void set_affinity(int i, pthread_t thid){
    CPU_ZERO(&CPU_masks[i]);
    CPU_SET(i, &CPU_masks[i]);

    int s = pthread_setaffinity_np(thid, sizeof(cpu_set_t), &CPU_masks[i]);

    if (s != 0){
        throw "err: pthread_setaffinity_np";
    }
}

void condx::wait(){
    pthread_mutex_lock(&mtx);
    pthread_cond_wait(&cond, &mtx);
    pthread_mutex_unlock(&mtx);
}
void condx::notify(){
    pthread_mutex_lock(&mtx);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mtx);
}

int get_random(int n){ // 0 ~ n
    srand((unsigned)time(NULL)); // 播种
    int ret = rand() % n;
    return ret;
}

int Gcd(int m,int n){
    int o;
    while(n>0){
        o=m%n;
        m=n;
        n=o;
    }
    return m;
}

void get_coprimes(vector<int>& arr, int n) {
    vector<int>().swap(arr); //清空元素

    for(int i=2;i<n;i++){
        int b=Gcd(i,n);
        if(b==1){
            arr.push_back(i);
        }
    }  
}

// void get_coprimes (vector<int>& arr,int n) {
//     vector<int>().swap(arr); //清空元素

//     for(int i=2;i<=n;i++){
//         bool flag = true;
//         for(int j=2;j<=i/2;j++){
//             if(i%j == 0) {
//                 flag = false;
//                 break;
//             }
//         }

//         if (flag && i != 2) {
//             arr.push_back(i);
//         }
//     }
// }

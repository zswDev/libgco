#ifndef __utils_h__
#define __utils_h__

#include <sys/time.h>
#include <time.h>
#include <sys/signal.h>
#include <sched.h>

#include <vector>

using namespace std;

extern int CPUS;

extern cpu_set_t* CPU_masks;

extern void cpu_init();

extern void set_affinity(int i, pthread_t thid);

extern int get_random(int n);

extern void get_coprimes (vector<int>& arr, int count);

class condx{
    private:
        pthread_cond_t cond;
        pthread_mutex_t mtx;
    public:
        void notify();
        void wait();
};

#endif
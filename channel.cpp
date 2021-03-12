#include <stdio.h>
#include <unistd.h>

#include <thread>

#include "channel.h"

int main(){
    channel<int> chan(8);

    thread t1([&]{
        usleep(40);
        for(int i=0;i<4;i++){
            usleep(30);
            chan<<i;
        }

        printf("t1 over\n");
    });

    thread t3([&]{
        for(int i=4;i<8;i++){
            usleep(30);
            chan<<i;
        }

        printf("t3 over\n");
    });

    //sleep(1);

    thread t2([&]{
        int i=0;

         usleep(10);
        for(;;){
            usleep(30);
            chan>>i;

            printf("t2: %i\n", i);
        }

        //printf("t2 over\n");
    });
    thread t4([&]{
        int i=0;

        for(;;){
            usleep(30);
            chan>>i;

            printf("t4: %i\n", i);
        }

        //printf("t2 over\n");
    });
  
    sleep(10);
    printf("over");
}
#include <iostream>
#include "mlist.h"

using namespace std;

int main(){
     mlist<int> m1;
     m1.push(1);
     m1.push(2);

     cout<<m1.pop(0)<<endl;
     cout<<m1.pop(0)<<endl;
     cout<<m1.pop(0)<<endl;
}
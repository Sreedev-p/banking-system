#include<iostream>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<new>
using namespace std;
class A{
    public:
        int data;
    A(int val){
        data = val;
    }
};
int main(){
    key_t key=1234;
    int shmid= shmget(key,sizeof(A),0666);
    void* ptr = shmat(shmid,NULL,0);
    A* obj=(A*)ptr;
    cout<<"value is "<<obj->data;
}
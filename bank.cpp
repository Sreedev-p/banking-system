#include<iostream>
#include<string>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<new>
#define key 191971
struct sharedClasses{
    depositClass deposit;
    withdrawClass withdraw;
    error ERROR;
};
class error{
    public:
    int errorCode;
}ERROR;
class depositClass{
    private:
        double amount;
        int accountNo;
    public:
        bool authenticate(){
            if(amount <0){
                return false;
            }
            return true;
        }
    depositClass(double amount, int accountNo){
        this->amount = amount;
        this->accountNo = accountNo;
    }
    depositClass(){
        amount =-1;//so that defaultly returns false (since amount =-1);
    }
};
class withdrawClass{
    private:
        double amount;
        int accountNo;
    public:
        bool authenticate(){
            // if(amount > balance || amount <0){
            //     return false;
            // }
            // return true;
        }
    withdrawClass(double amount,double balance, int accountNo){
        this->amount = amount;
        this->accountNo = accountNo;
    }
};
int main(){

    int shmid= shmget((key_t)key,sizeof(sharedClasses),0666|IPC_CREAT);
    sharedClasses* request = static_cast<sharedClasses*>(shmat(shmid,NULL,0));
}
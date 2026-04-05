#ifndef COMMON_H
#define COMMON_H
#include<sys/sem.h>
#include<sys/shm.h>
#include<sys/ipc.h>
#include<string>
#define semKey 124563
#define key 191971

enum OperationType { NONE, DEPOSIT, WITHDRAW ,SHOW,NEW};
enum channelType{INT,STRING,DOUBLE,EMPTY};
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};
class error{
    public:
    int errorCode;
}localERROR;
class depositClass{
    private:
        double amount;
        int accountNo;
    public:
    depositClass(double amount, int accountNo){
        this->amount = amount;
        this->accountNo = accountNo;
    }
    depositClass(){
        amount =-1;//so that defaultly returns false (since amount =-1);
        accountNo =0;
    }
};
class withdrawClass{
    private:
        double amount;
        int accountNo;
    public:
    withdrawClass(){
        amount=-1;
        accountNo=0;
    }
    withdrawClass(double amount,double balance, int accountNo){
        this->amount = amount;
        this->accountNo = accountNo;
    }
};
struct sharedClasses{
    depositClass deposit;
    withdrawClass withdraw;
    error ERROR;

    bool req;//request active or not
    channelType channel;
    int intChannel;
    char stringChannel[100];
    double doubleChannel;
    OperationType operation;//tell bank what operation the user is reuesting
    bool status;//tells if operation completed or not
    bool success;//tells if operation failed or nor
};

#endif
#include<iostream>
#include<new>
#include<fstream>
#include<sstream>
#include<vector>
#include<unordered_map>

using namespace std;


class userDetails{
    protected:  
        string name;
        int age;
        unsigned int accountNo;
        string password;
        double balance;
        string pin;
    public:
        string getName() const{
            return name;
        }
        double getBalance() const{
            return balance;
        }
        userDetails(unsigned int accountNumber,string name,int age,double balance,string password,string pin){
            accountNo = accountNumber;
            this->name =name;
            this->age =age;
            this->balance =balance;
            this->password=password;
            this->pin= pin;
        }
        userDetails(string name,int accountNo){
            this->name = name;
            this->accountNo = accountNo;
        }
        userDetails(){
            name = "";
        }
        ~userDetails(){

        }
};
void loadMap(unordered_map<int ,userDetails> &map){
    ifstream file("db.txt");
    if(!file.is_open()){
        localERROR.errorCode=201; //unable to open file
        return ;
    }
    string line;
    while(getline(file,line)){
        if(line.empty())
            continue;
        string accountNumberStr,name,ageStr,balanceStr,password,pin;
        stringstream stream(line);//convert the got line into a stream so that we can take it like a cin input
        getline(stream,accountNumberStr,',');//reads all csv to each variable
        getline(stream,name,',');
        getline(stream,ageStr,',');
        getline(stream,balanceStr,',');
        getline(stream,password,',');
        getline(stream,pin,',');
        unsigned int accountNumber = stoul(accountNumberStr);//convert vars from string to respective datatypes
        int age=stoi(ageStr);
        double balance=stod(balanceStr);
        userDetails temp(accountNumber,name,age,balance,password,pin);//create a temp userDetail object to feed the hashMap
        map[accountNumber]=temp;
    }
    file.close();
}
void printAllUsers(const unordered_map<int, userDetails> &bankMap) {
    cout << "--- Current Bank Database ---" << endl;
    for (const auto& [accNo, user] : bankMap) {
        cout << "Account: " << accNo 
        << " | Name: " << user.getName()
        << " | Balance: " << user.getBalance() 
        << endl;
    }
}
void system_init(sharedClasses* &request,int &shmId,int &semId){
    localERROR.errorCode =0;    //reset error value
    semId= semget((key_t)semKey,1,0666|IPC_CREAT);    //create semaphore
    if(semId==-1){
        localERROR.errorCode |= 1;
    }
    semun arg;     //create a semun union var
    arg.val= 1;
    if(semctl(semId,0,SETVAL, arg)== -1){
        localERROR.errorCode |= 2;
    }


    shmId= shmget((key_t)key,sizeof(sharedClasses),0666|IPC_CREAT);    //create and atach to the shared memory
    if(shmId == -1){
        localERROR.errorCode |=4;}
    else{
        void* ptr =shmat(shmId,NULL,0);
        if(ptr == (void*)-1)
            localERROR.errorCode |= 8;
        request = static_cast<sharedClasses*>(ptr);
    }

    request->req=false;    //initialise sharedStruct variables
    request->channel=EMPTY;
    request->operation=NONE;
    request->status=false;
    request->success=false;
    new(&(request->deposit)) depositClass();
    new(&(request->withdraw)) withdrawClass();
}

int main(){
    unordered_map<int,userDetails> map;
    sharedClasses *request;
    int shmId,semId;
    system_init(request,shmId,semId);//init shared mem and semaphores
    loadMap(map);//init the hashMap

}
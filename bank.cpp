#ifndef COMMON_H
#define COMMON_H
#include<sys/sem.h>
#include<sys/shm.h>
#include<sys/ipc.h>
#include<string>
#define SEM_KEY 5555
#define SHM_KEY 191971

enum OperationType { NONE, DEPOSIT, WITHDRAW ,SHOW,NEW,LOGIN};
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
    public:
        double amount;
        int accountNo;
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
    withdrawClass(double amount,int accountNo){
        this->amount = amount;
        this->accountNo = accountNo;
    }
};
struct sharedClasses{
    depositClass deposit;
    withdrawClass withdraw;
    error ERROR;

    bool req;//request active or not
    bool status;//tells if operation completed or not
    bool success;//tells if operation failed or nor

    channelType channel;
    int intChannel;
    char stringChannel[100];
    double doubleChannel;
    OperationType operation;//tell bank what operation the user is reuesting
};

#endif
#include<iostream>
#include<unistd.h>
#include<new>
#include<fstream>
#include<sstream>
#include<vector>
#include<unordered_map>
#include<errno.h>
#include<string.h>
#include<cstring>

using namespace std;

int shmId,semId;
class userDetails;

void processAnimation(int num,float speed);
void system_init(sharedClasses* &request);
void printAllUsers(const unordered_map<int, userDetails> &bankMap);
void loadMap(unordered_map<int ,userDetails> &map);
void wait(int semNum);
void post(int semNum);
void w(int semNum);
class userDetails{
    public:
        string name;
        int age;
        unsigned int accountNo;
        string password;
        double balance;
        string pin;
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

int main(){
    unordered_map<int,userDetails> users;
    sharedClasses *request;
    system_init(request);//init shared mem and semaphores
    cout<<localERROR.errorCode<<endl;

    cout<<"Initialising system";
    processAnimation(10,150);cout<<endl;sleep(1);

    loadMap(users);//init the hashMap
    cout<<"Loading user details";
    processAnimation(10,150);sleep(1);cout<<endl;
    cout<<"System ready for use "<<endl;

    //--------------------  Syys Init -------------------------------

    start: 
    wait(2); // wait for user requests 
    wait(1);// lock the mutex
    request->status =true;// means started working on it
    OperationType work = request->operation;
    channelType channel =request->channel;
    int accNo =request->intChannel;
    char permission;

    auto it =users.find(accNo);
    if(it == users.end()){
        request->ERROR.errorCode =402;
        request->success = false;
        post(1);//unlock mutex
        post(3);//wake up user
        wait(4);//wait user to read
        goto start;
    }
    userDetails current = users.at(accNo);

    if(work == DEPOSIT){
        double amount=request->deposit.amount;
        if(amount <= 0){
            request->ERROR.errorCode=403;
            post(1);//unlock mutex
            post(3);//signal user
            wait(4);//wait user to finish reading 
            goto start;
        }
        cout<<"Deposit\n"<<"-----------------------"<<endl;
        accNo =request->deposit.accountNo;
        cout<<"User: "<<accNo<<" Amount: "<<amount<<endl;
        cin.ignore(1000, '\n');
        cin>>permission;
        if(permission =='y' || permission == 'Y')request->success =true;
        else request->success =false;
        request->req= false;

        post(1);//unlock mutex
        post(3);//signal user
        wait(4);//wait user to finish
        goto start;
    }
    else if(work == WITHDRAW){

    }
    else if(work == NEW){

    }
    else if(work == SHOW){

    }
    else if(work == LOGIN){
        cout<<"User "<<accNo<<" is trying to login"<<endl;
        char pass[5];
        strcpy(pass,request->stringChannel);
        if(strncmp(current.pin.c_str(),pass,4)==0){
            cout<<"Allowed"<<endl;
            request->success =true;
        }
        else{
            cout<<"Refused"<<endl;
            request->success =false;
        }
        request->req =false;
        post(1);//exit mutex
        post(3);//signal user
        wait(4);//wait for user to read result
        goto start;
    }
    return 0;
}




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
void system_init(sharedClasses* &request){
    localERROR.errorCode =0;    //reset error value
    semId= semget((key_t)SEM_KEY,5,0666|IPC_CREAT);    //create semaphore
    if(semId==-1){
        localERROR.errorCode |= 1;
    }
    semun arg;     //create a semun union var
    arg.val= 1;
    if(semctl(semId,0,SETVAL, arg)== -1){ //mutex
        localERROR.errorCode |= 2;
    }
    else{ //four waiting semaphore
        if(semctl(semId,1,SETVAL, arg)== -1)
        localERROR.errorCode |= 2;
        else{
            arg.val = 0;
            if(semctl(semId,2,SETVAL, arg)== -1)
            localERROR.errorCode |= 2;
            else{
                if(semctl(semId,3,SETVAL, arg)== -1)
                localERROR.errorCode |= 2;
                else{
                    if(semctl(semId,4,SETVAL, arg)== -1)
                    localERROR.errorCode |= 2;
                }
            }
        }
    }

    shmId= shmget((key_t)SHM_KEY,sizeof(sharedClasses),0666|IPC_CREAT);    //create and atach to the shared memory
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

void w(int semNum){
    sembuf sb={semNum,0,SEM_UNDO};
    if(semop(semId,&sb,1)== -1){
        localERROR.errorCode= 301;
        return;
    }
}
void wait(int semNum) {
    struct sembuf sb;
    sb.sem_num = static_cast<unsigned short>(semNum); // Explicit C++ cast
    sb.sem_op = -1;
    sb.sem_flg = SEM_UNDO;

    if (semop(semId, &sb, 1) == -1) {
        perror("Wait Error"); // This will print why it failed
        localERROR.errorCode = 301;
    }
}

void post(int semNum) {
    struct sembuf sb;
    sb.sem_num = static_cast<unsigned short>(semNum);
    sb.sem_op = 1;
    sb.sem_flg = SEM_UNDO;

    if (semop(semId, &sb, 1) == -1) {
        perror("Post Error");
        localERROR.errorCode = 302;
    }
}
void processAnimation(int num,float speed){
    //num = no of dots speed is in milliseconds
    int mseconds = speed*1000;//conv to ms for usleep(microseconds)
    if(mseconds < 0)
        return;
    for(int i = 0; i < num; i++) {
        cout << "." << flush; 
        usleep(mseconds);       
    }
}

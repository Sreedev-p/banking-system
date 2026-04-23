//sem0->shmLock sem1->mutex  sem2->req  sem3->result sem4->ack
//should pass account number in the int channel
#ifndef COMMON_H
#define COMMON_H
#include<sys/sem.h>
#include<sys/shm.h>
#include<sys/ipc.h>
#include<string>
#define SEM_KEY 5555
#define SHM_KEY 5555

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
#include<regex>
#include<new>
#include<cstring>
using namespace std;
int shmId,semId;
sharedClasses* request;

void flushShm();
void processAnimation(int num,float speed);
int homepage();
void user_init();
void post(int semNum);
void wait(int semNum);
void w(int semNum);

class userDetails{
    protected:  
        string name;
        int age;
        unsigned int accountNo;
        string password;
        double balance;
        string pin;
    public:
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
        virtual ~userDetails(){

        }
};
class user: private userDetails{
    private:
        bool setPin(string pinVar){
            wait(0);//enter shm
            wait(1);//mutex lock
            // -> write into shm
            wait(1);
            post(2);//signal bank;
            wait(3);//wait for bank to finish
            wait(1);//lockk mutex
            //read from shm
            post(1);//unlock muttex
            post(4);//signal bank
            post(0);//exit shm
            const regex pattern("^[0-9]+$");//check if pinVar contains only digits
            if(pinVar.length()!= 4 ){//chec pin lenght == 4
                return false;
            }
            if (!regex_match(pinVar, pattern)) 
            return false;
            return std::stoll(pinVar) > 0;//chec if pinVar contained int is +ve
        }
        bool setPassword(string word){
            wait(0);//enter shm
            wait(1);//mutex lock
            // -> write into shm
            wait(1);
            post(2);//signal bank;
            wait(3);//wait for bank to finish
            wait(1);//lockk mutex
            //read from shm
            post(1);//unlock muttex
            post(4);//signal bank
            post(0);//exit shm
            if(word.length()<5){
                localERROR.errorCode=101;
                return false;
            }
            else{
                password = word;
            }
        }
        bool authenticate(int accountNo,string tempPin){
            wait(0);//lock shared memory
            wait(1);//mutex => write lock
            flushShm();
            request->channel= STRING;
            request->operation = LOGIN;
            strcpy(request->stringChannel,tempPin.c_str());
            request->intChannel =accountNo;
            post(2);//signal bank process
            post(1);
            wait(3);//wait for bank to complete
            wait(1);//mutex
            bool success =request->success;
            cout<<success<<endl;
            post(1);//unloc mutex
            post(4);//ack the bank
            post(0);//exit shm
            if(success){
            this->accountNo= accountNo;
            this->pin =tempPin;
            return success;
            }
            return success;
        }
        int assignAccountNumber(){
            wait(0);//enter shm
            wait(1);//mutex lock
            // -> write into shm
            wait(1);
            post(2);//signal bank;
            wait(3);//wait for bank to finish
            wait(1);//lockk mutex
            //read from shm
            post(1);//unlock muttex
            post(4);//signal bank
            post(0);//exit shm
            request->operation= NEW;//to get assigned new account number from bank
            //wait for status to be true;

        }
    public:
        bool deposit(double amount){
            wait(0);//enter shm
            wait(1);//lock mutex
            flushShm();
            request->operation = DEPOSIT;
            new(&(request->deposit)) depositClass(amount,this->accountNo);//writes amount and account no to shared memory deposit class
            request->channel= INT;
            request->intChannel =accountNo;
            post(2);//signal bank
            post(1);//unlock mutex
            wait(3);//wait for bank to finish
            wait(1);//lock mutex
            bool success= request->success;
            post(1);//unlock mutex
            post(4);//signal bank of read
            post(0);//exit shm
            return success;
            } 
 
        bool withdraw(double amount){
            wait(0);//enter shm
            wait(1);//mutex lock
            // -> write into shm
            flushShm();
            new(&(request->withdraw)) withdrawClass(amount,this->accountNo);
            request->operation =WITHDRAW;
            request->channel =INT;
            request->intChannel =this->accountNo;
            wait(1);
            post(2);//signal bank;
            wait(3);//wait for bank to finish
            wait(1);//lockk mutex
            //read from shm
            bool success =request->success;
            post(1);//unlock muttex
            post(4);//signal bank
            post(0);//exit shm//writes amount and account no to shared memory withdraw class
            return success;
        }
        int servicePage(){
        int choice;
        cout<<"Welcome "<<name<<endl;
        cout<<"-------------------------"<<endl;
        cout<<"1. Deposit "<<endl;
        cout<<"2. Withdraw "<<endl;
        cout<<"3. Show Details"<<endl;
        cout<<"4. Quit"<<endl;
        cout<<"Enter your choice"<<endl;
        cin>>choice;
        return choice;
        }
        bool login(int accountNo,string pin){
            return authenticate(accountNo,pin);
        }
        bool login(){
            int accountNoVar;
            string pinVar;
            cout<<"Enter your account No : ";
            cin>>accountNoVar;
            cout<<"Enter pin: ";
            cin.ignore(1000,'\n');
            getline(cin,pinVar);
            return authenticate(accountNoVar,pinVar);
        }
        bool createAccount(){
            string nameVar,passVar,pinVar;
            int ageVar;
            cout<<"Enter Your name: ";
            getline(cin,nameVar);
            cout<<"Enter your age: ";
            cin>>ageVar;
            if(age< 18){
                processAnimation(5,10);
                cout<<"Sorry ";
                return false;
            }
            name = nameVar;
            age = ageVar;

            
            accountNo = assignAccountNumber();
            processAnimation(4,10);
            cout<<"Congradulations you have Successfully created an account"<<endl;
            cout<<"Your Credentials "<<endl;
            cout<<"Name: "<<name<<endl;
            cout<<"Account Number: "<<accountNo<<endl;
            while(true){
                cout<<"Set password:";
                cin.ignore(1000,'\n');
                getline(cin,passVar);
                bool var = setPassword(passVar);
                if(!var){
                    processAnimation(5,20);
                    cout<<"Password too short"<<endl;
                    cout<<"Try again"<<endl;
                    continue;
                }
                else{
                    processAnimation(5,20);
                    cout<<"Successfully set password"<<endl;
                    break;
                }

            }
            while(true){
                cout<<"Set a four digit pin:";
                cin.ignore(1000,'\n');
                getline(cin,pinVar);
                bool var = setPin(passVar);
                if(!var){
                    processAnimation(5,20);
                    cout<<"error while setting pin ";
                    cout<<" Try again"<<endl;
                    continue;
                }
                else{
                    processAnimation(5,20);
                    cout<<"Successfully set pin"<<endl;
                    break;
                }

            }
            cout<<"Thank you for creating account"<<endl;
            cout<<"You can now login";
            processAnimation(6,30);
            cout<<endl;     
        }
};


int main(){
    user_init();
    user current;
    int choice;
    home:
    choice =homepage();
    bool isLogin,isUser;
    //--------------------- login ----------------------
    if(choice == 1){
        bool isLogin=current.login();
        if(! isLogin){
            cout<<"something went wrong !"<<endl;
            sleep(1);
            return 0;
        }
    }
    else if( choice == 2){
        if(!current.createAccount()){
            cout<<"Something went wrong"<<endl;
            return 0;
        }
        if(!current.login()){
            cout<<"Something went wrong"<<endl;
            return 0;
        }
    }
    else{
        cout<<"Thank you"<<endl;
        return 0;
    }

    //------------------  services ----------------------
    while(1){
    choice = current.servicePage();
        if(choice == 1){
            double amount;
            cout<<"DEPOSIT: "<<endl;
            cout<<"Enter Amount: ";cin>>amount;
            if(current.deposit(amount))cout<<"Deposite successfull "<<endl;
            else cout<<"Deposite failed"<<endl;
        }
        else if(choice == 2){
            double amount;
            cout<<"WITHDRAW: "<<endl;
            cout<<"Enter amount: ";cin>> amount;
            if(current.withdraw(amount))cout<<"Withdraw Successfull"<<endl;
            else cout<<"Withdraw failed "<<endl;
            sleep(1);
        }
        else if(choice == 4){
            return 0;
        }
    }

}


void user_init(){
    shmId = shmget((key_t)SHM_KEY,sizeof(sharedClasses),0666);
    void *ptr =shmat(shmId,NULL,0);
    request = static_cast<sharedClasses*>(ptr);

    semId= semget((key_t)SEM_KEY,5,0666);
    return;
}
void flushShm(){
    request->ERROR.errorCode=0;
    request->success =false;
    request->channel =EMPTY;
    request->operation =NONE;
}
void wait(int semNum){
    sembuf sb={semNum,-1,SEM_UNDO};
    if(semop(semId,&sb,1)== -1){
        localERROR.errorCode= 301;
        return;
    }
}
void post(int semNum){
    sembuf sb={semNum,1,SEM_UNDO};
    if(semop(semId,&sb,1)== -1){
        localERROR.errorCode= 302;
        return;
    }
}
void w(int semNum){
    sembuf sb={semNum,0,SEM_UNDO};
    if(semop(semId,&sb,1)== -1){
        localERROR.errorCode= 301;
        return;
    }
}
int homepage(){
    int choice;
    cout<<"Welcome to Sreedev's bank"<<endl;
    cout<<"-------------------------"<<endl;
    cout<<"To Login enter 1"<<endl;
    cout<<"Dont have an account? Enter 2"<<endl;
    cout<<"To quit enter 3"<<endl;
    cout<<"Enter your choice"<<endl;
    cin>>choice;
    return choice;
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
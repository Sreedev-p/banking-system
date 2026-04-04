#include<iostream>
#include<string>
#include<unistd.h>
#include<regex>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<new>
#define key 191971;
using namespace std;
struct sharedClasses{
    depositClass deposit;
    withdrawClass withdraw;
    error ERROR;
};
class error{
    public:
    int errorCode;
}ERROR;
//used to authenticate deposites
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
//to authenticate withdrawals
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
    withdrawClass(double amount,int accountNo){
        this->amount = amount;
        this->accountNo = accountNo;
    }
    withdrawClass(){
        amount =-1;//so that defaultly returns false (since amount =-1);
    }
};
class userDetails{
    protected:  
        string name;
        int age;
        unsigned int accountNo;
        string password;
        double balance;
        string pin;
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
class user: private userDetails{
    private:
        bool setPin(string pinVar){
            const regex pattern("^[0-9]+$");//check if pinVar contains only digits
            if(pinVar.length()!= 4 ){//chec pin lenght == 4
                return false;
            }
            if (!regex_match(pinVar, pattern)) 
            return false;
            return std::stoll(pinVar) > 0;//chec if pinVar contained int is +ve
        }
        bool deposit(double amount,sharedClasses* request){
            new(&(request->deposit)) depositClass(amount,this->accountNo);//writes amount and account no to shared memory deposit class
            if(request->deposit.authenticate()){//if authenticated deposits amount
                balance =balance+amount;//
                return true;
            }
            return false;
        }
        bool withdraw(double amount,sharedClasses* request){
            new(&(request->withdraw)) withdrawClass(amount,this->accountNo);//writes amount and account no to shared memory withdraw class
            if(request->withdraw.authenticate()){//if authenticated withdraws amount
                balance =balance-amount;
                return true;
            }
            return false;
        }
        bool setPassword(string word){
            if(word.length()<5){
                ERROR.errorCode=100;
                return false;
            }
            else{
                password = word;
            }
        }
        bool authenticate(int accountNo,string pin){
            if(this->accountNo == accountNo){
                if(this->pin == pin)
                    return true;
            }
            return false;
        }
        int assignAccountNumber(){
            return 20;
        }
    public:
        bool login(int accountNo,string pin){
            authenticate(this->accountNo,this->pin);
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
void processAnimation(int num,float speed){//num = no of dots speed is in milliseconds
    int mseconds = speed*1000;//conv to ms for usleep(microseconds)
    if(mseconds < 0)
        return;
    for(int i = 0; i < num; i++) {
        cout << "." << flush; 
        usleep(mseconds);       
    }
}
int homepage(){
    int choice;
    cout<<"Welcome to Sreedev's bank"<<endl;
    cout<<"-------------------------"<<endl;
    cout<<"To Login enter 1"<<endl;
    cout<<"Dont have an account? Enter 2"<<endl;
    cout<<"To quit enter 3"<<endl;
    cout<<"Enter your choice";
    cin>>choice;
    return choice;
}
int main(){
    user userVar;
    int choice;
    home:
    choice =homepage();
    switch(choice){
        case 1:
            userVar.login();
        case 2:
            userVar.createAccount();
        case 3:
            return 0;
        default:
            cout<<"Enter a valid choice"<<endl;
    }
}
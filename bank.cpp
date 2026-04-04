#include<iostream>
#include<string>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<new>
#include<fstream>
#include<sstream>
#include<vector>
#include<unordered_map>
#define key 191971
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
class error{
    public:
    int errorCode;
}localERROR;
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
struct sharedClasses{
    depositClass deposit;
    withdrawClass withdraw;
    error ERROR;
};



// Inside your Bank program
void printAllUsers(const unordered_map<int, userDetails> &bankMap) {
    cout << "--- Current Bank Database ---" << endl;
    
    // [key, value] is a structured binding
    for (const auto& [accNo, user] : bankMap) {
        cout << "Account: " << accNo 
             << " | Name: " << user.getName() // Assumes you have a getName() getter
             << " | Balance: " << user.getBalance() 
             << endl;
    }
}




int main(){
    unordered_map<int,userDetails> map;
    int shmid= shmget((key_t)key,sizeof(sharedClasses),0666|IPC_CREAT);
    sharedClasses* request = static_cast<sharedClasses*>(shmat(shmid,NULL,0));
    loadMap(map);
    printAllUsers(map);
}
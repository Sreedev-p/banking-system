#include<iostream>
#include<string>
using namespace std;
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
        amount =-1;//so that defaultly returns false if amount =-1;
    }
};
class withdrawClass{
    private:
        double amount;
        int accountNo;
    public:
        bool authenticate(){
            if(amount > balance || amount <0){
                return false;
            }
            return true;
        }
    withdrawClass(double amount,double balance, int accountNo){
        this->amount = amount;
        this->accountNo = accountNo;
    }
};
class userDetails{
    protected:  
        string name;
        unsigned int accountNo;
        string password;
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
        double balance;
        unsigned short pin;
        
        bool deposit(double amount){
            depositClass *tempDeposit = new depositClass(amount,this->accountNo);//asks for authentication from bank and returns T/F
            if(tempDeposit.authenticate()){//if authenticated deposits amount
                balance =balance+amount;
                return true;
            }
            return false;
        }
        bool withdraw(double amount){
            withdrawClass *tempWithdraw = new withdrawClass(amount,this->balance,this->accountNo);//asks for authentication from bank and returns T/F
            if(tempWithdraw.authenticate()){//if authenticated withdraws amount
                balance =balance-amount;
                return true;
            }
            return false;
        }
        bool setPassword(string word,){
            if(strlen(word)<5){
                ERROR.errorCode=100;
                return false;
            }
            else{
                
            }
        }
}
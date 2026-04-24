#ifndef COMMON_H
#define COMMON_H

#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <string>
#include<string.h>

#define SEM_KEY 6666
#define SHM_KEY 6666

// --- IPC Configuration & Data Structures ---

enum OperationType { NONE, DEPOSIT, WITHDRAW, SHOW, ADD, NEW, LOGIN };
enum channelType   { INT, STRING, DOUBLE, EMPTY };

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

class error {
public:
    int errorCode;
} localERROR;

class newUser{
    public:
        int accNo;
        char name[50];
        char password[20];
        char pin[5];
        int age;
    newUser(int a, const char n[], const char pi[], const char p[], int age ){
        this->accNo = a;
        this->age = age;
        strcpy(name, n);
        strcpy(password, p);
        strcpy(pin, pi);
    }
    newUser(){}
};
class depositClass {
public:
    double amount;
    int accountNo;

    depositClass(double amount, int accountNo) {
        this->amount = amount;
        this->accountNo = accountNo;
    }
    depositClass() {
        amount = -1; // Default failure state
        accountNo = 0;
    }
};

class withdrawClass {
public:
    double amount;
    int accountNo;

    withdrawClass() {
        amount = -1;
        accountNo = 0;
    }
    withdrawClass(double amount, int accountNo) {
        this->amount = amount;
        this->accountNo = accountNo;
    }
};

struct sharedClasses {
    depositClass deposit;
    withdrawClass withdraw;
    error ERROR;
    newUser user;
    bool success;

    channelType channel;
    int intChannel;
    char stringChannel[100];
    char stringChannel2[10];
    double doubleChannel;
    OperationType operation;
};

#endif

// --- Main Application Includes ---

#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include <regex>
#include <new>
#include <cstring>

using namespace std;

// Global IPC Handles
int shmId, semId;
sharedClasses* request;

// Function Prototypes
void flushShm();
void flushSem();
void* requestProcessAnimation(void *arg);
void processAnimation(int num, float speed);
int homepage();
void user_init();
void post(int semNum);
void wait(int semNum);
void w(int semNum);

// --- User Management Classes ---

class userDetails {
protected:
    string name;
    int age;
    unsigned int accountNo;
    string password;
    double balance;
    string pin;

public:
    userDetails(unsigned int accountNumber, string name, int age, double balance, string password, string pin) {
        accountNo = accountNumber;
        this->name = name;
        this->age = age;
        this->balance = balance;
        this->password = password;
        this->pin = pin;
    }
    userDetails(string name, int accountNo) {
        this->name = name;
        this->accountNo = accountNo;
    }
    userDetails() {
        name = "";
    }
    virtual ~userDetails() {}
};

class user : private userDetails {
private:
    bool setPin(string pinVar) {
        wait(0); // enter shm
        wait(1); // mutex lock
        // -> write into shm
        wait(1);
        post(2); // signal bank;
        wait(3); // wait for bank to finish
        wait(1); // lock mutex
        // read from shm
        post(1); // unlock mutex
        post(4); // signal bank
        post(0); // exit shm

        const regex pattern("^[0-9]+$");
        if (pinVar.length() != 4) return false;
        if (!regex_match(pinVar, pattern)) return false;
        
        return std::stoll(pinVar) > 0;
    }

    bool setPassword(string word) {
        wait(0);
        wait(1);
        wait(1);
        post(2);
        wait(3);
        wait(1);
        post(1);
        post(4);
        post(0);

        if (word.length() < 5) {
            localERROR.errorCode = 101;
            return false;
        } else {
            password = word;
            return true; 
        }
    }

    bool authenticate(int accountNo, string tempPin) {
        wait(0); // lock shared memory
        wait(1); // mutex => write lock
        
        flushShm();
        flushSem();

        request->channel = STRING;
        request->operation = LOGIN;
        strcpy(request->stringChannel, tempPin.c_str());
        request->intChannel = accountNo;

        post(2); // signal bank process
        post(1);
        wait(3); // wait for bank to complete
        wait(1); // mutex

        bool success = request->success;
        cout << success << endl;

        post(1); // unlock mutex
        post(4); // ack the bank
        post(0); // exit shm

        if (success) {
            this->accountNo = accountNo;
            this->pin = tempPin;
        }
        return success;
    }

    int assignAccountNumber() {
        wait(0);
        wait(1);

        request->operation = NEW;
        request->channel = INT;

        post(2);
        post(1);
        wait(3);
        wait(1);

        bool success = request->success;
        if(success){accountNo = request->intChannel;}
        
        post(1);
        post(4);
        post(0);

        if(success)return accountNo;
        return -1;
    }

public:
    bool deposit(double amount) {
        wait(0);
        wait(1);
        
        flushShm();
        flushSem();

        pthread_t tid;
        pthread_create(&tid, NULL, requestProcessAnimation, NULL);

        request->operation = DEPOSIT;
        new(&(request->deposit)) depositClass(amount, this->accountNo);
        request->channel = INT;
        request->intChannel = accountNo;

        post(2);
        post(1);
        wait(3);
        wait(1);

        bool success = request->success;
        
        post(1);
        post(4);
        post(0);

        pthread_cancel(tid);
        return success;
    }

    bool withdraw(double amount) {
        wait(0);
        wait(1);
        
        flushShm();
        flushSem();

        new(&(request->withdraw)) withdrawClass(amount, this->accountNo);
        request->operation = WITHDRAW;
        request->channel = INT;
        request->intChannel = accountNo;

        post(2);
        post(1);
        wait(3);
        wait(1);

        bool success = request->success;

        post(1);
        post(4);
        post(0);
        
        return success;
    }

    int servicePage() {
        int choice;
        cout << "\nWelcome " << name << endl;
        cout << "-------------------------" << endl;
        cout << "1. Deposit " << endl;
        cout << "2. Withdraw " << endl;
        cout << "3. Show Details" << endl;
        cout << "4. Quit" << endl;
        cout << "Enter your choice: ";
        cin >> choice;
        return choice;
    }

    bool login(int accountNo, string pin) {
        return authenticate(accountNo, pin);
    }

    bool login() {
        int accountNoVar;
        string pinVar;
        cout << "Enter your account No : ";
        cin >> accountNoVar;
        cout << "Enter pin: ";
        cin.ignore(1000, '\n');
        getline(cin, pinVar);
        return authenticate(accountNoVar, pinVar);
    }

    bool createAccount() {
        string nameVar, passVar, pinVar;
        int ageVar;

        cout << "Enter Your name: ";cin.ignore(1000,'\n');
        getline(cin, nameVar);
        cout << "Enter your age: ";
        cin >> ageVar;

        if (ageVar < 18) {
            processAnimation(5, 10);
            cout << "Sorry, you must be 18+";
            return false;
        }

        name = nameVar;
        age = ageVar;
        accountNo = assignAccountNumber();
        //<-------------------- trial ----------------------->
        cout<<"ACCNO : "<<accountNo;
        return false;
        //<--------------------------------------------------->
        processAnimation(4, 10);
        cout << "\nCongratulations! Successfully created an account" << endl;
        cout << "Name: " << name << " | Account Number: " << accountNo << endl;

        while (true) {
            cout << "Set password: ";
            cin.ignore(1000, '\n');
            getline(cin, passVar);
            if (!setPassword(passVar)) {
                processAnimation(5, 20);
                cout << "Password too short! Try again." << endl;
                continue;
            }
            processAnimation(5, 20);
            cout << "Successfully set password" << endl;
            break;
        }

        while (true) {
            cout << "Set a four digit pin: ";
            getline(cin, pinVar);
            if (!setPin(pinVar)) {
                processAnimation(5, 20);
                cout << "Error while setting pin! Try again." << endl;
                continue;
            }
            processAnimation(5, 20);
            cout << "Successfully set pin" << endl;
            break;
        }

        //<-------------- ADD ACCOUNT TO BANK DB ------------------>

        wait(0); wait(1);

        flushSem(); flushShm();

        new(&(request->user)) newUser((int)accountNo, nameVar.c_str(), passVar.c_str(), pinVar.c_str(), ageVar);
        request->operation = ADD;

        post(2); post(1); wait(3); wait(1);

        bool success = request->success;

        post(4); post(1); post(0);

        cout << "Thank you for creating an account. You can now login.";
        processAnimation(6, 30);
        cout << endl;
        return true;
    }
};

// --- Main Loop ---

int main() {
    user_init();
    user current;
    int choice;

    home:
    choice = homepage();
    
    if (choice == 1) {
        if (!current.login()) {
            cout << "Something went wrong!" << endl;
            sleep(1);
            return 0;
        }
    } else if (choice == 2) {
        current.createAccount();
        sleep(2);
    } else {
        cout << "Thank you" << endl;
        return 0;
    }

    while (1) {
        choice = current.servicePage();
        if (choice == 1) {
            double amount;
            cout << "DEPOSIT - Enter Amount: "; cin >> amount;
            if (current.deposit(amount)) cout << "Deposit successful" << endl;
            else cout << "Deposit failed" << endl;
            sleep(1);
        } else if (choice == 2) {
            double amount;
            cout << "WITHDRAW - Enter Amount: "; cin >> amount;
            if (current.withdraw(amount)) cout << "Withdraw successful" << endl;
            else cout << "Withdraw failed" << endl;
            sleep(1);
        } else if (choice == 3) {
            
        }else if (choice == 4){
            return 0;
        }
    }
}

// --- Utility Functions ---

void *requestProcessAnimation(void *arg) {
    string s = "Processing Request ............................................";
    for (int i = 0; s[i] != '\0'; i++) {
        cout << s[i] << flush;
        if (s[i] == '.') usleep(500000);
        else usleep(9000);
    }
    pthread_exit(NULL);
}

void user_init() {
    shmId = shmget((key_t)SHM_KEY, sizeof(sharedClasses), 0666);
    void *ptr = shmat(shmId, NULL, 0);
    request = static_cast<sharedClasses*>(ptr);
    semId = semget((key_t)SEM_KEY, 5, 0666);
}

void flushShm() {
    request->ERROR.errorCode = 0;
    request->success = false;
    request->channel = EMPTY;
    request->operation = NONE;
}

void flushSem() {
    while (semctl(semId, 3, GETVAL) > 0) wait(3);
    while (semctl(semId, 4, GETVAL) > 0) wait(4);
}

void wait(int semNum) {
    sembuf sb = {(unsigned short)semNum, -1, SEM_UNDO};
    if (semop(semId, &sb, 1) == -1) localERROR.errorCode = 301;
}

void post(int semNum) {
    sembuf sb = {(unsigned short)semNum, 1, SEM_UNDO};
    if (semop(semId, &sb, 1) == -1) localERROR.errorCode = 302;
}

void w(int semNum) {
    sembuf sb = {(unsigned short)semNum, 0, SEM_UNDO};
    if (semop(semId, &sb, 1) == -1) localERROR.errorCode = 301;
}

int homepage() {
    int choice;
    cout << "\nWelcome to Sreedev's Bank" << endl;
    cout << "-------------------------" << endl;
    cout << "1. Login" << endl;
    cout << "2. Create Account" << endl;
    cout << "3. Quit" << endl;
    cout << "Enter choice: ";
    cin >> choice;
    return choice;
}

void processAnimation(int num, float speed) {
    int mseconds = speed * 1000;
    if (mseconds < 0) return;
    for (int i = 0; i < num; i++) {
        cout << "." << flush;
        usleep(mseconds);
    }
}
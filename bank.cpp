#ifndef COMMON_H
#define COMMON_H

#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <string>
#include<string.h>

#define SEM_KEY 6666
#define SHM_KEY 6666

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
    newUser(int a, char n[], char p[], char pi[], int age ){
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
        amount = -1;
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

// --- Server-Side Includes ---

#include <iostream>
#include <unistd.h>
#include <new>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <errno.h>
#include <string.h>
#include <cstring>

using namespace std;

int shmId, semId;

// --- Function Prototypes ---

class userDetails;
void processAnimation(int num, float speed);
void system_init(sharedClasses* &request);
void printAllUsers(const unordered_map<int, userDetails> &bankMap);
void loadMap(unordered_map<int, userDetails> &map);
void wait(int semNum);
void post(int semNum);
void w(int semNum);

class userDetails {
public:
    string name;
    int age;
    unsigned int accountNo;
    string password;
    double balance;
    string pin;

    string getName() const    { return name; }
    double getBalance() const { return balance; }

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
    userDetails() { name = ""; }
    ~userDetails() {}
};

// --- Main Server Loop ---

int main() {
    unordered_map<int, userDetails> users;
    sharedClasses *request;

    // --- System Initialization ---
    system_init(request);
    cout << "Error Check Code: " << localERROR.errorCode << endl;

    cout << "Initialising system";
    processAnimation(10, 150); cout << endl; sleep(1);

    loadMap(users);
    cout << "Loading user details";
    processAnimation(10, 150); sleep(1); cout << endl;
    cout << "System ready for use " << endl;

    char permission;

    start:
    wait(2); // Wait for user request signal
    wait(1); // Lock mutex

    OperationType work = request->operation;
    int accNo = request->intChannel;

    // --- User Validation ---
    auto it = users.find(accNo);
    if (it == users.end() && work != NEW && work != ADD) {
        cout << "\nLOGIN REQ [" << accNo << "]" << endl;
        cout << "-----------------------------------" << endl;
        cout << "DENIED | INVALID ACC-NO" << endl;

        request->ERROR.errorCode = 402;
        request->success = false;

        post(1); // Unlock mutex
        post(3); // Signal user
        wait(4); // Wait for user ack
        goto start;
    }

    userDetails current;
    if(work != NEW && work != ADD)
        current = users.at(accNo);

    // --- Request Handling Logic ---

    if (work == DEPOSIT) {
        double amount = request->deposit.amount;
        if (amount <= 0) {
            request->ERROR.errorCode = 403;
            post(1); post(3); wait(4);
            goto start;
        }

        cout << "\nDEPOSIT REQ [" << accNo << "] | Balance: [" << current.balance << "]" << endl;
        cout << "-------------------------------------" << endl;
        cout << "Req Amount: " << amount << "\n[Y|N] : ";
        cin >> permission;

        request->success = (permission == 'y' || permission == 'Y');

        post(3); post(1); wait(4);
        goto start;

    } else if (work == WITHDRAW) {
        double amount = request->withdraw.amount;
        if (amount <= 0) {
            request->ERROR.errorCode = 403;
            post(1); post(3); wait(4);
            goto start;
        }

        cout << "\nWITHDRAW REQ [" << accNo << "] | Balance: [" << current.balance << "]" << endl;
        cout << "--------------------------------------" << endl;
        cout << "Req Amount: " << amount << "\n[Y|N] : ";
        cin >> permission;

        request->success = (permission == 'y' || permission == 'Y');

        post(3); post(1); wait(4);
        goto start;

    } else if (work == NEW) {

        int maxKey = 101;
        for( const auto &pair :users){
            if(pair.first > maxKey)
                maxKey = pair.first;
        }
        request->intChannel = maxKey + 1;
        cout<<"ACCNO: "<<maxKey;
        post(3); post(1); wait(4);
        goto start;

    } else if (work == SHOW) {
        post(3); post(1); wait(4);
        goto start;

    } else if (work == LOGIN) {
        char pass[100]; // Matches sharedClasses stringChannel size
        accNo = request->intChannel;

        cout << "\nLOGIN REQ [" << accNo << "]" << endl;
        cout << "-----------------------------------" << endl;
        strcpy(pass, request->stringChannel);

        if (strncmp(current.pin.c_str(), pass, 4) == 0) {
            cout << "STATUS: ALLOWED" << endl;
            request->success = true;
        } else {
            cout << "STATUS: DENIED | INVALID PIN" << endl;
            request->success = false;
        }

        post(3); post(1); wait(4);
        goto start;
    }

    return 0;
}

// --- Logic Implementations ---

void loadMap(unordered_map<int, userDetails> &map) {
    ifstream file("db.txt");
    if (!file.is_open()) {
        localERROR.errorCode = 201;
        return;
    }
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        string accountNumberStr, name, ageStr, balanceStr, password, pin;
        stringstream stream(line);
        
        getline(stream, accountNumberStr, ',');
        getline(stream, name, ',');
        getline(stream, ageStr, ',');
        getline(stream, balanceStr, ',');
        getline(stream, password, ',');
        getline(stream, pin, ',');

        unsigned int accountNumber = stoul(accountNumberStr);
        int age = stoi(ageStr);
        double balance = stod(balanceStr);

        userDetails temp(accountNumber, name, age, balance, password, pin);
        map[accountNumber] = temp;
    }
    file.close();
}

void printAllUsers(const unordered_map<int, userDetails> &bankMap) {
    cout << "--- Current Bank Database ---" << endl;
    for (const auto& [accNo, user] : bankMap) {
        cout << "Account: " << accNo 
             << " | Name: " << user.getName()
             << " | Balance: " << user.getBalance() << endl;
    }
}

void system_init(sharedClasses* &request) {
    localERROR.errorCode = 0;
    semId = semget((key_t)SEM_KEY, 5, 0666 | IPC_CREAT);
    if (semId == -1) localERROR.errorCode |= 1;

    semun arg;
    arg.val = 1;

    // Mutex (Sem 0) and Binary (Sem 1)
    if (semctl(semId, 0, SETVAL, arg) == -1) localERROR.errorCode |= 2;
    if (semctl(semId, 1, SETVAL, arg) == -1) localERROR.errorCode |= 2;

    // Waiting Semaphores (Sem 2, 3, 4)
    arg.val = 0;
    for (int i = 2; i <= 4; ++i) {
        if (semctl(semId, i, SETVAL, arg) == -1) localERROR.errorCode |= 2;
    }

    // --- Shared Memory Init ---
    shmId = shmget((key_t)SHM_KEY, sizeof(sharedClasses), 0666 | IPC_CREAT);
    if (shmId == -1) {
        localERROR.errorCode |= 4;
    } else {
        void* ptr = shmat(shmId, NULL, 0);
        if (ptr == (void*)-1) localERROR.errorCode |= 8;
        request = static_cast<sharedClasses*>(ptr);
    }

    request->channel = EMPTY;
    request->operation = NONE;
    request->success = false;
    new(&(request->deposit)) depositClass();
    new(&(request->withdraw)) withdrawClass();
    new(&(request->user)) newUser();
}

void wait(int semNum) {
    struct sembuf sb;
    sb.sem_num = static_cast<unsigned short>(semNum);
    sb.sem_op = -1;
    sb.sem_flg = SEM_UNDO;
    if (semop(semId, &sb, 1) == -1) {
        perror("Wait Error");
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

void w(int semNum) {
    sembuf sb = {(unsigned short)semNum, 0, SEM_UNDO};
    if (semop(semId, &sb, 1) == -1) {
        localERROR.errorCode = 301;
    }
}

void processAnimation(int num, float speed) {
    int mseconds = speed * 1000;
    if (mseconds < 0) return;
    for (int i = 0; i < num; i++) {
        cout << "." << flush;
        usleep(mseconds);
    }
}
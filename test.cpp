#include <iostream>
#include<pthread.h>
#include <unistd.h> // Required for usleep

using namespace std;
void *requestProcessAnimation(void *arg){
    string s="Processing Request ............................................";
    for(int i=0;s[i]!= '\0';i++){
        cout<<s[i]<<flush;
        if(s[i]=='.') usleep(50000);
        else usleep(9000);
    }
    pthread_exit(NULL);
}

int main(){
    pthread_t tid;
    cout<<flush;
    pthread_create(&tid,NULL,requestProcessAnimation,NULL);
    cout<<flush;
    sleep(10);
    pthread_join(tid,NULL);
    return 0;
}

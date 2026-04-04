#include <iostream>
#include <unistd.h> // Required for usleep
using namespace std;

int main() {
    for(int i = 0; i < 10; i++) {
        cout << "." << flush; // REMEMBER: flush is vital here!
        usleep(500000);       // Delay for 0.5 seconds
    }
    return 0;
}
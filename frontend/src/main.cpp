#include "Client.h"
#include <iostream>

int main() {
    hangman::Client client;
    if (!client.connectToServer("127.0.0.1", 5555)) {
        std::cerr << "Cannot connect to server\n";
        return 1;
    }
    client.run();
    return 0;
}

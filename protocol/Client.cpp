#include "Client.hpp"
#include "Application.hpp"

int main(){
    UserInterface user = UserInterface();
    string userInput;
    cout << "Input port to connect:" << endl;
    int port;
    cin >> port;
    std::cout << "port: " << port << std::endl;
    Connection* connection = user.CONNECT("127.0.0.1", port);
    
    thread sendThread([connection, user]() {
    sendLoop(connection, user);
    });
    thread receiveThread([connection, user]() {
    receiveLoop(connection, user);
    });

    sendThread.join();
    receiveThread.join();
    
    return 0;
}
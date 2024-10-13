#include "Server.hpp"
#include "Application.hpp"

int main(){
    UserInterface user = UserInterface();
    BlptSocket sock = user.LISTEN(55555);
    Connection* connection = new Connection(sock);

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


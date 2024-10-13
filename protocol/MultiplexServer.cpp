#include "MultiplexServer.hpp"
#include "Log.hpp"
#include <iostream>

int main(){
    UserInterface user = UserInterface();
    BlptSocket sockClient1 = user.LISTEN(55554);
    Connection* client1 = new Connection(sockClient1);
    BlptSocket sockClient2 = user.LISTEN(55555);
    Connection* client2 = new Connection(sockClient2);
    cout << &client1 << " " << &client2 << endl;
    thread t([client1, client2, user]() {
    loop(client1, client2, user);
    });

    t.join();
    
    return 0;
}

int loop(Connection* connection1, Connection* connection2, UserInterface userInterface){
    vector<uint8_t> receivedData;
    while(true){
        if (userInterface.STOREDBYTES(connection1) > 0){
            receivedData = userInterface.RECEIVE(connection1, 4);
            userInterface.SEND(connection2, receivedData);
        }
        if (userInterface.STOREDBYTES(connection2) > 0){
            receivedData = userInterface.RECEIVE(connection2, 4);
            userInterface.SEND(connection1, receivedData);
        }
    }
     
    return 0;
}
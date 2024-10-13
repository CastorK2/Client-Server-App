#include <cstdint>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "Connection.hpp"
#include "BlptSocket.hpp"
#include "UserInterface.hpp"
#include "Log.hpp"

using namespace std;

/* initializes UDP socket and creates Connection object
    * parameters:
    *  remote IP address
    *  remote port
    * returns:A
    *  connection if successful
    */
Connection* UserInterface::CONNECT(const char* remoteIP, int remotePort){
    //init logger
    logManager.initializeClient();
    // check port
    if (remotePort < 0 || remotePort > 65535){
        ERR("Invalid port");
        exit(EXIT_FAILURE);
    }
    
    //Declare Server address and init with 0;
    struct sockaddr_in serverAddr = {0};
    struct sockaddr_in thisAddr = {0};

    //Create UDP socket
    int clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if(clientSocket == -1){
        ERR("Socket create failed");
        exit(EXIT_FAILURE);
    }
    //Specify ServerAddress attributes
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(remotePort);

    in_addr_t ipAddress = inet_addr(remoteIP);
    if (ipAddress <= 0 || ipAddress == INADDR_BROADCAST || IN_MULTICAST(ipAddress)) {
        ERR("Invalid remote IP address");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }
    serverAddr.sin_addr.s_addr = ipAddress;
    BlptSocket* thisBlptSocket = new BlptSocket(thisAddr, serverAddr, clientSocket);
    // Client Connection Object
    Connection* thisConnection = new Connection(*thisBlptSocket, NONE);

    //Attempt connection with server
    //Perform 3-way handshake...

    GenericHeaderExtension handshake = { 
        .headerName = headerID::HANDSHAKE_HEADER,
        .length= {5, 0},
        .extensionData = {(uint8_t) headerID::NO_ADD_HEADER, (uint8_t) headerID::HANDSHAKE_HEADER, (uint8_t) headerID::FIN_RST_HEADER, 0b00000100, 0b00100100},
        .nextHeader = {0,0},
    };

    /* uint8_t headerAdded;
    do{
        headerAdded = thisConnection->addHeader(handshake);
    }while(headerAdded == 0); */

    thisConnection->addHeader(handshake);

    thisConnection->state = CLIENT_INIT;

    INFO("Client connecting to {}:{}", remoteIP, remotePort);
    INFO("Sending 1st Handshake message");

    // 1st Handshake Message is sent now
    
    //returns  the current address to which the socket sockfd is bound, in the buffer pointed to by addr.
    if (getsockname(thisConnection->blptSock.udpSocket, reinterpret_cast<struct sockaddr*>(&thisConnection->blptSock.thisAddr), &thisConnection->blptSock.thisAddrLen) == -1) {
        ERR("Error getting local address");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }
    return thisConnection;
}


/* creates socket object
    * parameters:
    *  local port
    *  local IP address
    * returns:
    *  blpt socket
    */
BlptSocket UserInterface::LISTEN(int localPort, const char* localIP){
    //init logger
    logManager.initializeServer();
    // check port
    if (localPort < 0 || localPort > 65535){
        ERR("Invalid port");
        exit(EXIT_FAILURE);
    }
    
    //Declare Server address and init with 0;
    struct sockaddr_in thisAddr = {0};
    struct sockaddr_in otherAddr = {0};

    //Create UDP socket on server side
    int thisFdSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if(thisFdSocket == -1){
        ERR("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    thisAddr.sin_family = AF_INET;
    thisAddr.sin_port = htons(localPort);
    in_addr_t ipAddress = inet_addr(localIP);
    thisAddr.sin_addr.s_addr = ipAddress;

    //Bind socket to thisAddr
    if (bind(thisFdSocket, (struct sockaddr*)&thisAddr, sizeof(thisAddr)) == -1) {
        ERR("Binding the socket failed");
        exit(EXIT_FAILURE);
    }
    BlptSocket thisBlptSocket = BlptSocket(thisAddr, otherAddr, thisFdSocket);
    INFO("Now listening on {} and {}", localIP, localPort);
    return thisBlptSocket;
}

/* creates socket object
    * parameters:
    *  local port
    * returns:
    *  blpt socket
    */
BlptSocket UserInterface::LISTEN(int localPort){
    //init logger
    logManager.initializeServer();
    // check port
    if (localPort < 0 || localPort > 65535){
        ERR("Invalid port");
        exit(EXIT_FAILURE);
    }
    
    //Declare Server address and init with 0;
    struct sockaddr_in thisAddr = {0};
    struct sockaddr_in otherAddr = {0};

    //Create UDP socket on server side
    int thisFdSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if(thisFdSocket == -1){
        ERR("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    thisAddr.sin_family = AF_INET;
    thisAddr.sin_port = htons(localPort);
    thisAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    //Bind socket to thisAddr
    if (bind(thisFdSocket, (struct sockaddr*)&thisAddr, sizeof(thisAddr)) == -1) {
        ERR("Binding the socket failed");
        exit(EXIT_FAILURE);
    }
    BlptSocket thisBlptSocket = BlptSocket(thisAddr, otherAddr, thisFdSocket);
    INFO("Now listening on {}", localPort);
    return thisBlptSocket;
}

/* calls send of Connection referenced by local connection identifier
    * parameters:
    *  local connection identifier
    *  data
    * returns:
    *  bytes currently in process
    *  error else
    */
int UserInterface::SEND(Connection* connection, vector<uint8_t> data){
    // Check if connection is open
    if (connection->state == CLOSE_WAIT || connection->state == EOF_WAIT || connection->state == HALF_CLOSED || connection->state == CLOSED){
        ERR("Send on closed connection");
        exit(EXIT_FAILURE);
    }
    // queue bytes
    connection->currentSendBufferQueue.insert(connection->currentSendBufferQueue.end(), data.begin(), data.end());
    connection->highestUserByte += data.size();
    if (connection->currentSendBufferQueue.empty()){
        return 0;
    }else{
        return connection->currentSendBufferQueue.size();
    }
}

/* receives at most size bytes from the given connection
    * parameters:
    *  connection
    *  size of bytes to receive
    * returns:
    *  data
    */
vector<uint8_t> UserInterface::RECEIVE(Connection* connection, long unsigned int size) {
    long unsigned int remainingsize = size;
    vector<uint8_t> bufferWeReadOut = {};
    if(!connection->currentReceiveBufferQueue.empty()){
        if(connection->currentReceiveBufferQueue.size() >= size){
            bufferWeReadOut = vector<uint8_t>(connection->currentReceiveBufferQueue.begin(), connection->currentReceiveBufferQueue.begin() + size);
            connection->currentReceiveBufferQueue.erase(connection->currentReceiveBufferQueue.begin(), connection->currentReceiveBufferQueue.begin() + size);
        }else{
            remainingsize = remainingsize - connection->currentReceiveBufferQueue.size();
            bufferWeReadOut = vector<uint8_t>(connection->currentReceiveBufferQueue.begin(), connection->currentReceiveBufferQueue.end());
            connection->currentReceiveBufferQueue.erase(connection->currentReceiveBufferQueue.begin(), connection->currentReceiveBufferQueue.end());
        }
        
    }
    return bufferWeReadOut;
}

/* returns number of bytes the user can fetch from the given connection
    * parameters:
    *  connection
    * returns
    *  numer of bytes
    */
size_t UserInterface::STOREDBYTES(Connection* connection){
    return connection->currentReceiveBufferQueue.size();
}

bool UserInterface::OTHERWANTSCLOSE(Connection* connection){
    return connection->state == EOF_ || connection->state == CLOSED || connection->state == EOF_WAIT;
}

/*
    * parameters:
    *  local connection
    * returns:
    *  error if connection is not open
    *  0 else
    */
int UserInterface::CLOSE(Connection* connection){
    if (connection->state == CLOSED){
        ERR("Closing the Connection when already closed");
        exit(EXIT_FAILURE);
    }
    if (connection->state == CLOSE_WAIT || connection->state == EOF_WAIT || connection->state == HALF_CLOSED){
        ERR("Closing the Connection when already trying to close");
        exit(EXIT_FAILURE);
    }
    if (connection->state == CLIENT_INIT || connection->state == HALF_OPEN || connection->state == SERVER_INIT || connection->state == LISTEN_){
        ERR("Closing unestablished Connection");
        exit(EXIT_FAILURE);
    }
    WARN("Closing connection");
    connection->close();
    return 0;
}

void UserInterface::USE_SECURITY(Connection* connection){
    connection->initSecurity();
}
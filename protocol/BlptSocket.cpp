#include <sys/socket.h>
#include "BlptSocket.hpp"
#include "Log.hpp"

using namespace std;

BlptSocket::BlptSocket(sockaddr_in _thisAddr, sockaddr_in _otherAddr, int _udpSocket){
    thisAddr = _thisAddr;
    otherAddr = _otherAddr;
    udpSocket = _udpSocket;
    thisAddrLen = sizeof(_thisAddr);
    otherAddrLen = sizeof(_otherAddr);
}

// Returns how many bytes have actually been sent and not just the payload
int BlptSocket::send(vector<uint8_t>messageBuffer){
    size_t messageSize = messageBuffer.size();
    uint8_t* messageArray = &messageBuffer[0];

    int bytesSent = sendto(udpSocket, messageArray, messageSize, MSG_DONTWAIT, (const struct sockaddr *) &otherAddr, otherAddrLen);
    if (bytesSent == -1){
        return -1;
    }
    return bytesSent;
}

vector<uint8_t> BlptSocket::receive(){
    int bytesRec = recvfrom(udpSocket, receiveBuffer, RECV_BUFFER_SIZE, MSG_DONTWAIT, (struct sockaddr *) &otherAddr, &otherAddrLen);
    if(bytesRec == -1){
        return {0xff};
    }
    vector<uint8_t> receivedMessage(receiveBuffer, receiveBuffer + bytesRec);
    return receivedMessage;
}
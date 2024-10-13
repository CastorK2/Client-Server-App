#pragma once
#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include <cstdint>
#include "Logger.hpp"
#include "Connection.hpp"
#include "BlptSocket.hpp"

class UserInterface{
public:
    Logger logManager;

    Connection* CONNECT(const char* remoteIP, int remotePort);
    BlptSocket LISTEN(int localPort, const char* localIP);
    BlptSocket LISTEN(int localPort);
    int SEND(Connection* connection, vector<uint8_t> data);
    vector<uint8_t> RECEIVE(Connection* connection, long unsigned int size);
    size_t STOREDBYTES(Connection* connection);
    bool OTHERWANTSCLOSE(Connection* connection);
    int CLOSE(Connection* connection);
    void USE_SECURITY(Connection* connection);
};

#endif //USER_INTERFACE_H
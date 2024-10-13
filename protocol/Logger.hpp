#pragma once
#ifndef LOGGER_H
#define LOGGER_H

class Logger{
private:
    bool initialized;
public:
    Logger();
    void initializeServer();
    void initializeClient();
};


#endif //LOGGER_H
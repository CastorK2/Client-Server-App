#pragma once
#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <thread>
#include "UserInterface.hpp"

int sendLoop(Connection* thisConnection);
int receiveLoop(Connection* thisConnection);

#endif //SERVER_H
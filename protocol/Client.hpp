#pragma once
#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <thread>
#include "UserInterface.hpp"

int sendLoop(Connection* thisConnection);
int receiveLoop(Connection* thisConnection);

#endif //CLIENT_H
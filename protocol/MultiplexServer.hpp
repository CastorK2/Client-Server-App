#pragma once
#ifndef MULTIPLEX_SERVER_H
#define MULTIPLEX_SERVER_H

#include <thread>
#include "UserInterface.hpp"

int loop(Connection* connection1, Connection* connection2, UserInterface userInterface);

#endif //MULTIPLEX_SERVER_H
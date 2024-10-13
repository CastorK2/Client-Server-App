#pragma once
#ifndef APPLICATION_H
#define APPLICATION_H
#include <fstream>

int sendLoop(Connection* thisConnection, UserInterface userInterface){
    string userInput;
    while(thisConnection->state != CLOSED){
        cout << "Input command to run:" << endl;
        getline(cin, userInput);
        if(userInput == "SEND"){
            getline(cin, userInput);
            vector<uint8_t> sendMessageVector(userInput.begin(), userInput.end());

            userInterface.SEND(thisConnection, sendMessageVector);
        }else if(userInput == "SEND BEE"){
            // inspired by https://www.tutorialspoint.com/how-to-read-a-text-file-with-cplusplus
            std::ifstream beeFile("beemovie_testfile.txt" , std::ios::binary);
            if (!beeFile) {
                std::cout << "Couldn't find the Beemovie File" << std::endl;
                continue;
            }
            std::vector<uint8_t> beeScript((std::istreambuf_iterator<char>(beeFile)), std::istreambuf_iterator<char>());
            beeFile.close();
            userInterface.SEND(thisConnection, beeScript);
        } else if (userInput == "SEC"){
            thisConnection->security_on = true;

        }else if (userInput == "CLOSE"){
            userInterface.CLOSE(thisConnection);
        }
    }
    return 0;
}

int receiveLoop(Connection* thisConnection, UserInterface userInterface){
    while(true){
        vector<uint8_t> receivedData = userInterface.RECEIVE(thisConnection, 4);
        for (uint8_t thisByte : receivedData) {
                std::cout << static_cast<char>(thisByte) << std::flush;
        }
    }
    std::cout << std::endl << std::endl;
    return 0;
}

#endif //APPLICATION_H
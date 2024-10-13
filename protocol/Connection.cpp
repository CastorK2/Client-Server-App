#include <sodium.h>
#include <iostream>
#include "Connection.hpp"
#include "Log.hpp"
#include <sodium.h>

using namespace std;

Connection::Connection(BlptSocket &_blptSock): blptSock(_blptSock){
    open = PASSIVE;
    finSeq = INIT_FIN_SEQ;
    handSeq = INIT_HAND_SEQ;
    state = LISTEN_;
    sendSeq = INIT_SEQ_NUMBER;
    sendPlaceInBuffer = INIT_SEQ_NUMBER;
    highestUserByte = INIT_SEQ_NUMBER;
    retransmissionSqn = INIT_SEQ_NUMBER;
    sendAck = INIT_SEQ_NUMBER;
    receivedSeq = INIT_SEQ_NUMBER;
    receivedAck = INIT_SEQ_NUMBER;
    oldReceivedAck =INIT_SEQ_NUMBER;
    ReceivedAckPure =INIT_SEQ_NUMBER;
    oldReceivedAckPure =INIT_SEQ_NUMBER;
    receivedAckGREhandSeq = false;
    receivedAckGREfinSeq = false;
    sendPlaceInBufferGREReceivedAck = false;
    sqnWrapped = false;
    headersToSendSize = 0;
    //congestion control
    ssthresh = 4294967295;
    ownWindow = INITIAL_WINDOW;
    otherWindow = INITIAL_WINDOW;
    ownWindowUsed = ownWindow;
    handshankeLost = false;
    //reset timer
    rstStartTime = chrono::system_clock::now();
    rstCreated = false;
    //retransmission timer
    // We compute FlightSize with sendSeq - receivedAck + 1
    rto = 1000;
    timeout = rto;
    rttStartTime = chrono::system_clock::now();
    rttEndTime = chrono::system_clock::now();
    rttRunning = false;
    rttCreated = false;
    firstMeasurement = false;
    backoffCounter = 0;
    //measurements = {};
    // send and receive
    sendThread = new thread(&Connection::sendLoop, this);
    receiveThread = new thread(&Connection::receiveLoop, this);
    ackNeeded = false;
    // Sets up the initial key to decode the very first security extension header message
    const char *awesomeKey = "PIX ist SUPER!! PIX ist SUPER!!"; 
    memcpy(security_ext_key, awesomeKey, crypto_secretbox_KEYBYTES);
    security_on = false;
    securitySupported = false;
    publicKeySent = false;
    keyReceived = false;
    onBothSides = false;
    crypto_box_keypair(our_ecdh_public_key, our_ecdh_private_key);
}

Connection::Connection(BlptSocket &_blptSock, ConnectionState conState): blptSock(_blptSock){
    open = ACTIVE;
    finSeq = INIT_FIN_SEQ;
    handSeq = INIT_HAND_SEQ;
    state = conState;
    sendSeq = INIT_SEQ_NUMBER;
    sendPlaceInBuffer = INIT_SEQ_NUMBER;
    retransmissionSqn = INIT_SEQ_NUMBER;
    sendAck = INIT_SEQ_NUMBER;
    receivedSeq = INIT_SEQ_NUMBER;
    receivedAck = INIT_SEQ_NUMBER;
    oldReceivedAck =INIT_SEQ_NUMBER;
    ReceivedAckPure =INIT_SEQ_NUMBER;
    oldReceivedAckPure =INIT_SEQ_NUMBER;
    receivedAckGREhandSeq = false;
    receivedAckGREfinSeq = false;
    sendPlaceInBufferGREReceivedAck = false;
    sqnWrapped = false;
    headersToSendSize = 0;
    //congestion control
    ssthresh = 4294967295;
    ownWindow = INITIAL_WINDOW;
    otherWindow = INITIAL_WINDOW;
    ownWindowUsed = ownWindow;
    handshankeLost = false;
    //reset timer
    rstStartTime = chrono::system_clock::now();
    rstCreated = false;
    //retransmission timer
    // We compute FlightSize with sendSeq - receivedAck + 1
    rto = 1000;
    timeout = rto;
    rttStartTime = chrono::system_clock::now();
    rttEndTime = chrono::system_clock::now();
    rttRunning = false;
    rttCreated = false;
    firstMeasurement = false;
    backoffCounter = 0;
    // send and receive
    sendThread = new thread(&Connection::sendLoop, this);
    receiveThread = new thread(&Connection::receiveLoop, this);
    ackNeeded = false;
    // Sets up the initial key to decode the very first security extension header message
    const char *awesomeKey = "PIX ist SUPER!! PIX ist SUPER!!"; 
    memcpy(security_ext_key, awesomeKey, crypto_secretbox_KEYBYTES);
    security_on = false;
    securitySupported = false;
    publicKeySent = false;
    keyReceived = false;
    onBothSides = false;
    crypto_box_keypair(our_ecdh_public_key, our_ecdh_private_key);
}

void Connection::sendLoop(){
    while(true){
        if (state != NONE){
            send();
            this_thread::sleep_for(chrono::milliseconds(300));
        }
    }
}

void Connection::receiveLoop(){
    while(true){
        receive();
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

void Connection::rstLoop(){
    while (true){
        if (chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - rstStartTime).count() >= timeout){
            ownWindow = min(INITIAL_WINDOW, (int) ownWindow);
        }
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

void Connection::rstStart(){
    if (!rstCreated){
        rst = new thread(&Connection::rstLoop, this);
        rstCreated = true;
    }
    rstStartTime = chrono::system_clock::now();
}

void Connection::rttLoop(){
    while (true){
        if (rttRunning && rttElapsedMilliseconds() >= timeout){
            retransmit();
        }
        this_thread::sleep_for(chrono::milliseconds(G));
    }
}

void Connection::rttStart(){
    if(!rttCreated){
        rtt = new thread(&Connection::rttLoop, this);
        rttCreated = true;
    }
    timeout = rto;
    rttStartTime = chrono::system_clock::now();
    rttRunning = true;
}

void Connection::rttStop(){
    rttEndTime = chrono::system_clock::now();
    rttRunning = false;
}

double Connection::rttElapsedMilliseconds(){
    chrono::time_point<chrono::system_clock> endTime;
    if(rttRunning){
        endTime = chrono::system_clock::now();
    }else{
        endTime = rttEndTime;
    }
    return chrono::duration_cast<chrono::milliseconds>(endTime - rttStartTime).count();
}
    
void Connection::updateState(Message message){
    switch(state){
    case NONE:
        break;
    case LISTEN_: //state before SERVER_INIT
        if(message.hasHandshakeHeader()){
            INFO("Server Received 1st HS message");
            state = SERVER_INIT;
            GenericHeaderExtension handshake = { 
                .headerName = headerID::HANDSHAKE_HEADER,
                .length= {5, 0},
                .extensionData = {(uint8_t) headerID::NO_ADD_HEADER, (uint8_t) headerID::HANDSHAKE_HEADER, (uint8_t) headerID::FIN_RST_HEADER, 0b00000100, 0b00100100},
                .nextHeader = {0,0},
            };
            addHeader(handshake);
            INFO("Sending 2nd Handshake message ")
            ackNeeded = true;
        }
        break;
    case CLIENT_INIT:
        if (message.getAckNumber() > sendSeq + 1){
            reset(message.getAckNumber(), INVALID_PACKET);
        }
        if(message.hasHandshakeHeader() && message.getAckNumber() >= handSeq){
            INFO("Client received 2nd HS message");
            state = ESTABLISHED;
            INFO("Sending 3rd handshake message");
            ackNeeded = true;
        }else{
            if (message.getAckNumber() >= handSeq){
                state = HALF_OPEN;
            }
            if (message.hasHandshakeHeader()){
                state = SERVER_INIT;
                ackNeeded = true;
            }
        }
        break;
    case SERVER_INIT:
        if (message.getAckNumber() > sendSeq + 1){
            reset(message.getAckNumber(), INVALID_PACKET);
        }
        if (message.getAckNumber() >= handSeq){
            state = ESTABLISHED;
        }
        break;
    case HALF_OPEN:
        if (message.hasHandshakeHeader()){
            state = ESTABLISHED;
            ackNeeded = true;
        }
        break;
    case ESTABLISHED:
        if(message.hasFinRstHeader()){
            state = EOF_;
            ackNeeded = true;
        }
        break;
    case CLOSE_WAIT:
        if (finSeq != -1 && receivedAckGREfinSeq && message.hasFinRstHeader()){
            state = CLOSED;
            ackNeeded = true;
        }
        if (message.hasFinRstHeader()){
            state = EOF_WAIT;
            ackNeeded = true;
        }
        if (receivedAckGREfinSeq){
            state = HALF_CLOSED;
        }
        break;
    case EOF_WAIT:
        if (finSeq != -1 && receivedAckGREfinSeq){ //sqn, die nicht erreicht werden kann
            state = CLOSED;
        }
        break;
    case EOF_:
        break;
    case HALF_CLOSED:
        if(message.hasFinRstHeader()){
            state = CLOSED;
            ackNeeded = true;
        }
        break;
    case CLOSED:
        if (!message.hasRst()){
            reset(message.getAckNumber(), INVALID_PACKET);
        }
        break;
    }
    if (state == CLOSED){
        INFO("Connection successfully closed")
    }
    INFO("New state: {}", state);
}


/*
    * parameters:
    *  bytestream to send
    *  possibly more
    * returns:
    *  byteNumber if successful
    *  0 else
    */
int Connection::send(){
    if (!mutex.try_lock()){
        return 0;
    }

    int bytesSent = 0;
    if (ackNeeded || headersToSendSize > 0 || sendPlaceInBuffer != highestUserByte){
        INFO("ackNeeded: {}, headersToSendSize > 0: {}, sendPlaceInBuffer: {}, highestUserByte: {}", ackNeeded, headersToSendSize > 0, sendPlaceInBuffer, highestUserByte);
        ackNeeded = false;
        int sendWindow;
        if (currentSendBufferQueue.empty()){
            bytesSent = transferToMessageAndSend({});
        }else{
            sendWindow = min(min(ownWindowUsed, otherWindow), (int) currentSendBufferQueue.size() - sendPlaceInBuffer);
            sendWindow = max(sendWindow, 0);
            /* if (sendWindow <= 0){
                return 0;
            } */
            if (sendWindow > 0){
                if (!rttRunning){
                    rttStart();
                }
                // only take measurements for not retransmitted packets
                // first case of retransmission: sendSqn <= retransmissionSqn, if receivedAck - 1 <= sendPlaceInBuffen at retransmission
                // second case is more complicated, if receivedAck -1 > sendPlaceInBuffen at retransmission
                // here sqn will warp during retransmission, so we need another variable
                // this is a conservative implementation i.e. it can happen that we do not take measurements for normal packets
                if (!((sendSeq <= retransmissionSqn && sendPlaceInBufferGREReceivedAck) || (((sendSeq > retransmissionSqn && !sqnWrapped) || (sendSeq < retransmissionSqn && sqnWrapped)) && !sendPlaceInBufferGREReceivedAck))){
                    newMeasurement(sendSeq);
                }
            }
            vector<uint8_t> restOfMessage(currentSendBufferQueue.begin() + sendPlaceInBuffer, currentSendBufferQueue.begin() + sendPlaceInBuffer + sendWindow);
            bytesSent = transferToMessageAndSend(restOfMessage);
            if(bytesSent > 0){
                INFO("number of transmitted bytes: {}", restOfMessage.size());
                uint32_t oldSendSeq = sendSeq;
                sendSeq += restOfMessage.size() % SEQ_NUMBER_RANGE;
                sqnWrapped = oldSendSeq > sendSeq;
                sendPlaceInBuffer += restOfMessage.size();
                INFO("sendPlaceInBuffer after update: {}", sendPlaceInBuffer);
            }
        }
        if(bytesSent > 0){
            otherWindow -= bytesSent;
            ownWindowUsed -= bytesSent;
        }
    }
    mutex.unlock();
    return bytesSent;
}

int Connection::transferToMessageAndSend(vector<uint8_t>messageBuffer){
    Message message = Message(sendSeq, sendAck, min(currentReceiveBufferQueue.max_size() - currentReceiveBufferQueue.size(), (size_t) 1073741823), headersToSendSize, headersToSend, messageBuffer);
    //ERR("headersToSendSize: {}", headersToSendSize);    
    vector<uint8_t> bufferToBeSent = message.createByteStream();
    if(open == ACTIVE) onBothSides = true;
    if (security_on && onBothSides && !message.hasSecurityHeader()){
        // Encrypt the message which will be sent
        // First turn vector into char*
        const char* rawBuffer = reinterpret_cast<const char*>(bufferToBeSent.data());
        unsigned long long rawBufferLen = static_cast<unsigned long long>(bufferToBeSent.size());
        // Then create a nonce to prevent replay attacks
        unsigned char nonce[crypto_secretbox_NONCEBYTES];
        randombytes_buf(nonce, sizeof nonce);
        unsigned char cipheredMsg[rawBufferLen + crypto_secretbox_MACBYTES];
        crypto_secretbox_easy(cipheredMsg, (const unsigned char *)rawBuffer, rawBufferLen, nonce, security_ext_key);
        bufferToBeSent.clear();
        bufferToBeSent.insert(bufferToBeSent.end(), nonce, nonce + sizeof(nonce));
        bufferToBeSent.insert(bufferToBeSent.end(), cipheredMsg, cipheredMsg + sizeof(cipheredMsg));
    }
    else if (security_on && open == PASSIVE && !message.hasSecurityHeader()){
        onBothSides = true;
    }
    int bytesSent = blptSock.send(bufferToBeSent);
    // handle async
    if(bytesSent > 0){
        if(message.hasHandshakeHeader()){
            //ERR("Has send handshake");
            if (handSeq != INIT_HAND_SEQ){
                INFO("Handshake lost");
                handshankeLost = true;
            }
            handSeq = sendSeq;
            sendSeq = (sendSeq + 1) % SEQ_NUMBER_RANGE;
        }
        if (message.hasFinRstHeader()){
            finSeq = sendSeq;
            sendSeq = (sendSeq + 1) % SEQ_NUMBER_RANGE;
        }
        headersToSendSize = 0;
        INFO("Message send sqn: {} ack: {} win: {} handshake: {} fin: {} rst: {} on {}", message.getSeqNumber(), message.getAckNumber(), message.getRecWindow(), message.hasHandshakeHeader(), message.hasFin(), message.hasRst(), blptSock);
    }
    return bytesSent;
}

int Connection::receive(){
    if (!mutex.try_lock()){
        return 0;
    }
    vector<uint8_t> messageObjByteStream = blptSock.receive();
    // handle async
    if (messageObjByteStream[0] == 0xff && messageObjByteStream.size() == 1){
        mutex.unlock();
        return 0;
    }
    // Decipher msg
    if (security_on){
        WARN("sec on")
        // Take the nonce out of the vector the message which will be sent
        unsigned char received_nonce[crypto_secretbox_NONCEBYTES];
        copy(messageObjByteStream.begin(), messageObjByteStream.begin() + crypto_secretbox_NONCEBYTES, received_nonce);
        messageObjByteStream.erase(messageObjByteStream.begin(), messageObjByteStream.begin() + crypto_secretbox_NONCEBYTES);
        // Now copy encrypted message to a new array:
        int messageLen = messageObjByteStream.size();
        unsigned char encryptedMessage[messageLen];
        copy(messageObjByteStream.begin(), messageObjByteStream.end(), encryptedMessage);
        messageObjByteStream.erase(messageObjByteStream.begin(), messageObjByteStream.end());
        // Now decrypt the message
        unsigned char decryptedMessage[messageLen - crypto_secretbox_MACBYTES];
        if (crypto_secretbox_open_easy(decryptedMessage, encryptedMessage, messageLen, received_nonce, security_ext_key) != 0) {
            // Handle decryption failure (e.g., tampered data or wrong key)
            std::cout << "Error decrypting the message" << endl;
            return -1;
        }
        // Put decrypted Message back into the vector:
        messageObjByteStream.insert(messageObjByteStream.end(), decryptedMessage, decryptedMessage + sizeof(decryptedMessage));
    }
    if (securitySupported && open == PASSIVE && !security_on){
        initSecurity();
    }
    // Turn the byte Stream into message object and check whether a correct message was received
    int bytesReceived;
    Message thisMsg = Message();
    if (thisMsg.readByteStream(messageObjByteStream) == 1){
        ERR("Problem parsing the received message");
        reset(sendSeq, INVALID_PACKET);
        bytesReceived = -1;
    }else{
        bytesReceived = thisMsg.getData().size();
        if(keyReceived && publicKeySent && open == ACTIVE) {
            security_on = true;
        }
    }
    // start reset timer
    rstStart();
    INFO("Message received sqn: {} ack: {} win: {} handshake: {} fin: {} rst: {} on {}", thisMsg.getSeqNumber(), thisMsg.getAckNumber(), thisMsg.getRecWindow(), thisMsg.hasHandshakeHeader(), thisMsg.hasFin(), thisMsg.hasRst(), blptSock);
    // Update rtt
    checkMeasurement(thisMsg.getAckNumber());
    if (receivedAck < thisMsg.getAckNumber()){
        rttStart();
    }
    if (thisMsg.getAckNumber() == sendSeq){
        rttStop();
    }else if (state == ESTABLISHED || state == EOF_ || state == EOF_WAIT || state == CLOSE_WAIT || state == HALF_CLOSED) {
        // other endpoint has send wrong ack number, need to tell him right number
        ackNeeded = true;
    }

    // Update Ack Number the other one is waiting for
    oldReceivedAck = receivedAck;
    receivedAck = thisMsg.getAckNumber();
    oldReceivedAckPure = ReceivedAckPure;
    if((receivedAck > handSeq) && (handSeq != INIT_HAND_SEQ)){
        receivedAckGREhandSeq = true;
    }
    if((receivedAck > finSeq) && (finSeq != INIT_FIN_SEQ)){
        receivedAckGREfinSeq = true;
    }
    INFO("receivedAckGREhandSeq: {}, receivedAckGREfinSeq: {}", receivedAckGREhandSeq, receivedAckGREfinSeq);
    ReceivedAckPure = receivedAck - receivedAckGREfinSeq - receivedAckGREhandSeq;
    
    // do not update cwnd in handshake
    if (!thisMsg.hasHandshakeHeader() && handSeq != INIT_HAND_SEQ){
        congestionControl();
    }
    // set cwnd to SMSS if handshake was lost
    if (handshankeLost && receivedAckGREhandSeq){
        ownWindow = MTU;
    }
    
    // Update otherWindow in Connection: 
    otherWindow = thisMsg.getRecWindow();

    // Check if the packet is in order
    if(thisMsg.getSeqNumber() == sendAck){
        if(thisMsg.hasSecurityHeader()){
            std::cout << "MSG has SEC Header" << endl;
            keyReceived = true;
            if (securitySupported && !security_on){
            initSecurity();
            }

            vector<uint8_t> key = thisMsg.getData();
            if (key.size() != 32)
            {
                cout << "Error in reading the key" << endl;
                return -1;
            }
            keyReceived = true;
            
            
            copy(key.begin(), key.end(), other_ecdh_public_key);
            // This calculates the shared secret which we will then use as our symmetric key
            // if(open == ACTIVE) crypto_scalarmult(security_ext_key, our_ecdh_public_key, other_ecdh_public_key);
            // if(open == PASSIVE) crypto_scalarmult(security_ext_key, other_ecdh_public_key, our_ecdh_public_key);
            if (crypto_scalarmult(security_ext_key, our_ecdh_private_key, other_ecdh_public_key) != 0){
                ERR("There was a problem deriving the shared secret");
            }
        }
        currentReceiveBufferQueue.insert(currentReceiveBufferQueue.end(), thisMsg.data.begin(), thisMsg.data.end());
        sendAck = (sendAck + thisMsg.data.size()) % SEQ_NUMBER_RANGE;
        if(thisMsg.hasHandshakeHeader()){
            sendAck = (sendAck + 1) % SEQ_NUMBER_RANGE;
            //check if security extension is supported
            vector<uint8_t> data = thisMsg.getHeader(headerID::HANDSHAKE_HEADER).extensionData;
            for (size_t i = 0; i < data.size() - 1; ++i){
                if (data[i] == 0b00000100 && data[i+1] == 0b00100100){
                    securitySupported = true;
                }
            }
        }
        
        if(thisMsg.hasFinRstHeader()){
            sendAck = (sendAck + 1) % SEQ_NUMBER_RANGE;
            if(thisMsg.hasFin()){
            }else{
                WARN("Received RST");
                state = CLOSED;
            }
        }
        updateState(thisMsg);
        // send ACK for all correct packages that are no ACKs themself
        if (thisMsg.data.size() != 0){
            INFO("acknowledging data");
            ackNeeded = true;
        }
        trimSendBuffer();
    }else{
        //send message with ack = oldSeq i.e. currend sendAck
        ackNeeded = true;
    }
    mutex.unlock();
    return bytesReceived;
}

void Connection::retransmissionTimer(uint32_t r){
    backoffCounter = 0;
    if (!firstMeasurement){
        firstMeasurement = true;
        srtt = r;
        rttvar = r/2;
        rto = max((double) G, 4*rttvar);
    }else{
        rttvar = 3/4*rttvar + 1/4*abs(srtt-r);
        srtt = 7/8*srtt + 1/8*r;
        rto = srtt + max((double) G, 4*rttvar);
    }
    if (rto < 1000){
        rto = 1000;
    }
}

void Connection::retransmit(){
    INFO("Retransmitting");
    // use go-back-n
    retransmissionSqn = sendPlaceInBuffer;
    sendPlaceInBufferGREReceivedAck = sendPlaceInBuffer >= ((receivedAck - 1) % SEQ_NUMBER_RANGE);
    sendPlaceInBuffer = (receivedAck - 1) % SEQ_NUMBER_RANGE;//TODO: passt das? fin, handshake?
    // setting new rtt
    rto = 2*rto;

    rttStart();
    // update cwnd
    ownWindow = 2*MTU;
    if (backoffCounter == 0){
        ssthresh = max((int) (sendSeq - receivedAck + 1)/2, 2*MTU);
    }
    backoffCounter += 1;
    if (backoffCounter > 2){
        firstMeasurement = true;
    }
    if (!receivedAckGREhandSeq && (state == LISTEN_ || state == CLIENT_INIT || state == SERVER_INIT) && rto < 3000){
        rto = 3000;
    }
}

//implements congestion control as suggested by rfc5681
void Connection::congestionControl(){
    INFO("Old cwnd: {} rwnd: {}", ownWindow, otherWindow);
    if (ownWindow < ssthresh){
        slowStart();
    }else{
        congestionAvoidence();
    }
    ownWindowUsed = ownWindow;
    INFO("Updated cwnd: {} rwnd: {}", ownWindow, otherWindow);
}

void Connection::slowStart(){
    ownWindow += min((long) MTU, (receivedAck - oldReceivedAck) % SEQ_NUMBER_RANGE);
}

void Connection::congestionAvoidence(){
    if ((receivedAck - oldReceivedAck) % SEQ_NUMBER_RANGE > 0){
        if ((uint32_t) MTU*MTU/ownWindow == 0){
            ownWindow += 1;
        }else{
            ownWindow += (uint32_t) MTU*MTU/ownWindow;
        }
    }
}
    
// adds header to headersToSend
// nextHeader of header can be {0, 0}
void Connection::addHeader(GenericHeaderExtension header){
    if (headersToSendSize > 0){
        headersToSend[headersToSendSize].nextHeader[0] = (uint32_t) header.headerName;
        headersToSend[headersToSendSize].nextHeader[1] = 3 - ((__builtin_clz((int) header.headerName | 1) - 2) / 8);
    }
    headersToSend[headersToSendSize] = header;
    headersToSendSize++;
}

void Connection::trimSendBuffer(){
    int64_t bytesAcked = ReceivedAckPure - oldReceivedAckPure;
    INFO("bytesAcked: {}, ReceivedAckPure: {}, oldReceivedAckPure: {}", bytesAcked, ReceivedAckPure, oldReceivedAckPure);
    if(bytesAcked > 0){
        currentSendBufferQueue.erase(currentSendBufferQueue.begin(), currentSendBufferQueue.begin() + bytesAcked);
        sendPlaceInBuffer -=  bytesAcked;
        highestUserByte -= bytesAcked;
    }
}

void Connection::reset(uint32_t seqNumber, ErrorCode code){
    INFO("Resetting connection");
    GenericHeaderExtension rst = {
        .headerName = headerID::FIN_RST_HEADER,
        .length = {1, 0},
        .extensionData = {(uint8_t) code},
        .nextHeader = {0, 0}};
    addHeader(rst);

    Message message = Message(seqNumber, sendAck, ownWindow, headersToSendSize, headersToSend, {});
    vector<uint8_t> bufferToBeSent = message.createByteStream();

    int bytesSent = blptSock.send(bufferToBeSent);
    if(bytesSent >= 0){
        headersToSendSize = 0;
    }
    INFO("Message send sqn: {} ack: {} win: {} handshake: {} fin: {} rst: {}", message.getSeqNumber(), message.getAckNumber(), message.getRecWindow(), message.hasHandshakeHeader(), message.hasFin(), message.hasRst());
}

void Connection::close(){
    GenericHeaderExtension fin = {
        .headerName = headerID::FIN_RST_HEADER,
        .length = {1, 0},
        .extensionData = {0}, 
        .nextHeader = {0, 0}};
    
    addHeader(fin);
    finSeq = WANT_TO_SEND_FIN;

    if (state == EOF_){
        state = EOF_WAIT;
    }else if(state == ESTABLISHED){
        state = CLOSE_WAIT;
    }
    INFO("New state: {}", state);
}

void Connection::newMeasurement(uint32_t sqn){
    Measurement measurement = {.timer = Timer(),.seqNumber=sqn};
    measurement.timer.start();
    measurements.push_back(measurement);
    INFO("Created new measurement");
}

void Connection::checkMeasurement(uint32_t ack){
    vector<Measurement> tmpMeasurements = {};
    double measured = 0;
    for (uint i=0; i < measurements.size(); ++i){
        if (ack > measurements[i].seqNumber){
            measurements[i].timer.stop();
            measured = max(measured, measurements[i].timer.elapsedMilliseconds());
            measurements.erase(measurements.begin() + i);
        }
    }
    if (measured > 0){
        INFO("Measurement taken: {}", measured);
        retransmissionTimer(measured);
    }
}



int Connection::initSecurity(){
    if(open == PASSIVE && !publicKeySent){
        std::vector<uint8_t> key(our_ecdh_public_key, our_ecdh_public_key + crypto_box_PUBLICKEYBYTES);
        currentSendBufferQueue.insert(currentSendBufferQueue.end(), key.begin(), key.end());
        highestUserByte += crypto_box_PUBLICKEYBYTES;

        GenericHeaderExtension security = { 
        .headerName = headerID::SECURITY_EXTENSION,
        .length= {0, 0},
        .extensionData = {},
        .nextHeader = {0,0},
        };
        addHeader(security);
        publicKeySent = true;

        WARN("publicKeySent: {}, keyReceived: {}", publicKeySent, keyReceived);
    }
    else if(open == ACTIVE && keyReceived && !publicKeySent){

        std::vector<uint8_t> key(our_ecdh_public_key, our_ecdh_public_key + crypto_box_PUBLICKEYBYTES);
        currentSendBufferQueue.insert(currentSendBufferQueue.end(), key.begin(), key.end());
        highestUserByte += crypto_box_PUBLICKEYBYTES;

        GenericHeaderExtension security = { 
        .headerName = headerID::SECURITY_EXTENSION,
        .length= {0, 0},
        .extensionData = {},
        .nextHeader = {0,0},
        };
        addHeader(security);
        publicKeySent = true;

        WARN("publicKeySent: {}, keyReceived: {}", publicKeySent, keyReceived);
    }
    else if(open == PASSIVE && publicKeySent && keyReceived){
        INFO("Security Mode now activated");
        security_on = true;
        ackNeeded = true;

    }
    else{
        WARN("publicKeySent: {}, keyReceived: {}", publicKeySent, keyReceived);
        return 0;
    }

    
    return 0;
}

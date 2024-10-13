#pragma once
#ifndef CONNECTION_H
#define CONNECTION_H

#define BUFFER_SIZE 65536
#define INIT_SEQ_NUMBER 0
#define MAX_DATA_SIZE 65535
#define MTU 1500 //used as SMSS
#define INITIAL_WINDOW 3*MTU
#define RESTART_WINDOW 32
#define G 2000 // clock granularity
#define DELAY 300 // send and receive delay
#define SEQ_NUMBER_RANGE 4294967296
#define INIT_HAND_SEQ -2
#define INIT_FIN_SEQ -2
#define WANT_TO_SEND_FIN -1

#include <vector>
#include <cstdint>
#include <mutex>
#include <thread>
#include <chrono>
#include <sodium.h>
#include "Message.hpp"
#include "BlptSocket.hpp"
#include "Timer.hpp"
#include <sodium.h>

enum Open{
    PASSIVE,
    ACTIVE
};

enum ConnectionState{
    NONE,
    LISTEN_,
    CLIENT_INIT,
    SERVER_INIT,
    HALF_OPEN,
    ESTABLISHED,
    CLOSE_WAIT,
    EOF_WAIT,
    EOF_,
    HALF_CLOSED,
    CLOSED
};

enum ErrorCode{
    ORDINARY_SHUTDOWN = 0,
    UNKOWN = 1,
    INVALID_PACKET = 2,
    NO_CONNECTION = 3
};

typedef struct{
    Timer timer;
    uint32_t seqNumber;
}Measurement;

class Connection{
public:
    Open open;
    BlptSocket &blptSock;
    std::mutex mutex;
    vector<uint8_t> currentSendBufferQueue;
    vector<uint8_t> currentReceiveBufferQueue;
    ConnectionState state;

    //all measured in milliseconds
    double srtt; //smoothed round-trip time
    double rttvar; //round-trip time variation
    double rto; //retransmission timeout
    double timeout; //current timeout used by thread
    // reset timer
    thread* rst;
    bool rstCreated;
    std::chrono::time_point<std::chrono::system_clock> rstStartTime;
    // retransmssion timer
    std::chrono::time_point<std::chrono::system_clock> rttStartTime;
    std::chrono::time_point<std::chrono::system_clock> rttEndTime;
    bool rttRunning;
    thread* rtt;
    bool rttCreated;
    bool firstMeasurement;
    //Timer rtt; //retransmission timer
    int backoffCounter; // number of successive back offs
    vector<Measurement> measurements;
    vector<uint32_t> retransmissions;
    
    uint32_t ownWindow; //cwnd
    uint32_t ownWindowUsed; //remaining part of cwnd
    uint32_t otherWindow;  //rwnd
    uint32_t maximumSegmentSize;
    uint32_t ssthresh;
    bool handshankeLost;
    
    int64_t finSeq; // sqn of packet with which we have send a fin
    int64_t handSeq; // sqn of packet with which we have send a handshake
    uint32_t sendSeq; //highest sequence number send
    uint32_t sendPlaceInBuffer;
    uint32_t highestUserByte;
    uint32_t retransmissionSqn;
    uint32_t sendAck; //bytenumber we wait for
    uint32_t receivedSeq; //highest sequence number received
    uint32_t receivedAck; //bytenumber the other endpoint waits for
    uint32_t oldReceivedAck;//bytenumber the other endpoint waited for, 1 message before the newest message,
    uint32_t ReceivedAckPure; //bytenumber the other endpoint waited for, without counting the header
    uint32_t oldReceivedAckPure; //bytenumber the other endpoint waited for, 1 message before the newest message, without counting the header
    bool receivedAckGREhandSeq;
    bool receivedAckGREfinSeq;
    // variables needed to only take measurements of not retransmited packets
    bool sendPlaceInBufferGREReceivedAck;
    bool sqnWrapped; // has sendSeq wapped since last retransmit?
    // append headers to message
    GenericHeaderExtension headersToSend[MAX_NUM_HEADERS];
    uint8_t headersToSendSize;
    // thread management for send and receive
    thread* sendThread;
    thread* receiveThread;
    bool ackNeeded;
    // Security Extension attributes:
    // Key used for Security Extension
    uint8_t security_ext_key[crypto_secretbox_KEYBYTES];
    // Bool to check whether messages need to be integrity checked and decrypted/ encrypted or not
    bool security_on;
    bool securitySupported;
    bool publicKeySent;
    bool keyReceived;
    bool onBothSides;
    // Keys and Secret for ECDH key exchange
    unsigned char our_ecdh_public_key[crypto_box_PUBLICKEYBYTES];
    unsigned char our_ecdh_private_key[crypto_box_SECRETKEYBYTES];
    unsigned char other_ecdh_public_key[crypto_box_PUBLICKEYBYTES];
    


    Connection(BlptSocket &_blptSock, ConnectionState conState);
    Connection(BlptSocket &_blptSock);
    // thread management for send and receive
    void sendLoop();
    void receiveLoop();
    // functions to manage rst thread
    void rstLoop();
    void rstStart();
    // functions to manage rtt thread
    void rttLoop();
    void rttStart();
    void rttStop();
    double rttElapsedMilliseconds();

    int send();
    int transferToMessageAndSend(vector<uint8_t>messageBuffer);
    int receive();
    void retransmissionTimer(uint32_t r);
    void retransmit();
    void congestionControl();
    void slowStart();
    void congestionAvoidence();
    void displayAttributes() const;
    void addHeader(GenericHeaderExtension header);
    void updateState(Message message);
    void trimSendBuffer();
    void reset(uint32_t seqNumber, ErrorCode code);
    void close();
    void newMeasurement(uint32_t sqn);
    void checkMeasurement(uint32_t ack);
    int initSecurity();
    int noMutexSend();
    int noMutexReceive();
};
#endif //CONNECTION_H
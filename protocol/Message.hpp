#pragma once
#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstdint>
#include <optional>
#include <vector>
#include <cstdlib>
#include <sodium.h>

#define MAX_NUM_HEADERS 4
using namespace std;

enum class headerID{
    NO_ADD_HEADER = 0,
    HANDSHAKE_HEADER = 1,
    FIN_RST_HEADER = 2,
    SECURITY_EXTENSION = 1060,

    DEBUG = 1073741821,
};

typedef struct{
    headerID headerName;
    uint32_t length[2];
    vector<uint8_t> extensionData;
    uint32_t nextHeader[2];
}GenericHeaderExtension;


class Message{
public:
    // Mandatory Base Header
    // Schreibweise im Array: [actual Sequence number, Size in bytes]
    uint32_t seqNumber[2]; //fixedSizeInt should be 4
    uint32_t ackNumber[2]; //fixedSizeInt should be 4
    uint32_t recWindow[2]; //varSizeInt
    uint32_t nextHeader[2];// should be consistent with additionalHeaders[0]
    // Number of extension Headers
    uint32_t numAdditionalHeaders;
    // Sorted Array of Extension Headers
    GenericHeaderExtension additionalHeaders[MAX_NUM_HEADERS];

    // data
    vector<uint8_t> data;
    // security extension
    

    Message(uint32_t SeqNumber, uint32_t AckNumber, uint32_t RecWindow, uint32_t NumAdditionalHeaders, GenericHeaderExtension AdditionalHeaders[], vector<uint8_t> Data);
    Message();
    bool hasHandshakeHeader();
    bool hasFinRstHeader();
    bool hasSecurityHeader();
    bool hasFin();
    bool hasRst();
    GenericHeaderExtension getHeader(const headerID& header);
    uint32_t getSeqNumber();
    uint32_t getAckNumber();
    uint32_t getRecWindow();
    uint32_t getNextHeader();
    vector<uint8_t> getData();
    vector<uint8_t> createByteStream();
    vector<uint8_t> appendVariableHeaderField(vector<uint8_t> v, uint32_t num[2]);
    vector<uint8_t> appendFixedHeaderField(vector<uint8_t> v, uint32_t num[2]);
    uint64_t readByteStream(vector<uint8_t> byteStream);
    uint32_t transform8to32bit(vector<uint8_t> byteStream, uint8_t offset);
    uint32_t* readVariableLengthInteger(vector<uint8_t> byteStream, uint8_t offset);
};


#endif
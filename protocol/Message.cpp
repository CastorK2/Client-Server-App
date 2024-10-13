#include "Message.hpp"
#include <arpa/inet.h>
#include <sodium.h>
#include "Log.hpp"
#include <sodium.h>


using namespace std;

Message::Message(uint32_t SeqNumber, uint32_t AckNumber, uint32_t RecWindow, uint32_t NumAdditionalHeaders, GenericHeaderExtension AdditionalHeaders[], vector<uint8_t> Data){
    seqNumber[0] = SeqNumber;
    seqNumber[1] = 3;
    ackNumber[0] = AckNumber;
    ackNumber[1] = 3;
    recWindow[0] = RecWindow;
    recWindow[1] = 3 - ((__builtin_clz(RecWindow | 1) - 2) / 8); // calculate needed size
    if (NumAdditionalHeaders > 0){
        nextHeader[0] = (uint32_t)AdditionalHeaders[0].headerName;
        nextHeader[1] = 3 - ((__builtin_clz((int) AdditionalHeaders[0].headerName | 1) - 2) / 8);
    }else{
        nextHeader[0] = 0;
        nextHeader[1] = 0;
    }
    numAdditionalHeaders = NumAdditionalHeaders;
    GenericHeaderExtension empty = GenericHeaderExtension();
    for (uint8_t i=0;i<MAX_NUM_HEADERS;++i){
        if (i < NumAdditionalHeaders){
            additionalHeaders[i] = AdditionalHeaders[i];
        }else{
            additionalHeaders[i] = empty;
        }
    }
    data = Data;
}

Message::Message(){
    seqNumber[0] = 0;
    seqNumber[1] = 0;
    ackNumber[0] = 0;
    ackNumber[1] = 0;
    recWindow[0] = 0;
    recWindow[1] = 0;
    nextHeader[0] = 0;
    nextHeader[1] = 0;
    numAdditionalHeaders = 0;
    GenericHeaderExtension empty = GenericHeaderExtension();
    for (uint8_t i=0;i<MAX_NUM_HEADERS;++i){
            additionalHeaders[i] = empty;
    }
    data = {};
}

bool Message::hasHandshakeHeader(){
    bool res = false;
    for(int i = 0; i<MAX_NUM_HEADERS; ++i){
        res = res | (additionalHeaders[i].headerName == headerID::HANDSHAKE_HEADER);
    }
    return res;
}

bool Message::hasFinRstHeader(){
    bool res = false;
    for(int i = 0; i<MAX_NUM_HEADERS; ++i){
        res = res | (additionalHeaders[i].headerName == headerID::FIN_RST_HEADER);
    }
    return res;
}

bool Message::hasSecurityHeader(){
    bool res = false;
    for(int i = 0; i<MAX_NUM_HEADERS; ++i){
        res = res | (additionalHeaders[i].headerName == headerID::SECURITY_EXTENSION);
    }
    return res;
}



bool Message::hasFin(){
    if (!hasFinRstHeader()){
        return false;
    }
    return getHeader(headerID::FIN_RST_HEADER).extensionData.empty() || getHeader(headerID::FIN_RST_HEADER).extensionData[0] == 0;
}

bool Message::hasRst(){
    if (!hasFinRstHeader()){
        return false;
    }
    return !(getHeader(headerID::FIN_RST_HEADER).extensionData.empty() || getHeader(headerID::FIN_RST_HEADER).extensionData[0] == 0);
}

GenericHeaderExtension Message::getHeader(const headerID& header){
    for(int i = 0; i<MAX_NUM_HEADERS; ++i){
        if (additionalHeaders[i].headerName == header) {
            return additionalHeaders[i];
        }
    }
    ERR("No respective header found");
    exit(EXIT_FAILURE);
}

uint32_t Message::getSeqNumber(){
    return seqNumber[0];
}

uint32_t Message::getAckNumber(){
    return ackNumber[0];
}

uint32_t Message::getRecWindow(){
    
    return recWindow[0];
}

uint32_t Message::getNextHeader(){
    return nextHeader[0];
}

vector<uint8_t> Message::getData(){
    return data;
}

// Method for allocating the bytestream in memory (here, Big Endian is used)
vector<uint8_t> Message::createByteStream(){
    //uint64_t headerSize = calculateHeaderSize();
    vector<uint8_t> byteStream = {};
    //uint64_t currentHeaderMax = headerSize;
    // Add Sequence Number to byteStream on most significant Position
    byteStream = appendFixedHeaderField(byteStream, seqNumber);
    // Add Ack Number to byteStream on 2nd most significant Position
    byteStream = appendFixedHeaderField(byteStream, ackNumber);
    // Add receiving Window to byteStream on 3rd most significant Position
    byteStream = appendVariableHeaderField(byteStream, recWindow);
    // Add next Header
    byteStream = appendVariableHeaderField(byteStream, nextHeader);
    
    // Add optional headers
    if (nextHeader[0] != 0){
        for (u_int32_t i = 0; i < numAdditionalHeaders; ++i) {
            byteStream = appendVariableHeaderField(byteStream, additionalHeaders[i].length);
            byteStream.insert(byteStream.end(), additionalHeaders[i].extensionData.begin(), additionalHeaders[i].extensionData.end());
            byteStream = appendVariableHeaderField(byteStream, additionalHeaders[i].nextHeader);
        }
    }

    // Add data
    byteStream.insert(byteStream.end(), data.begin(), data.end());

    return byteStream;
}

vector<uint8_t> Message::appendFixedHeaderField(vector<uint8_t> v, uint32_t num[2]){
    uint32_t res = htonl(num[0]); //convert to network byte order
    for (int i = num[1]; i >= 0; i--){
        v.push_back((res >> (8*i)) & 0xff);
    }
    return v;
}

vector<uint8_t> Message::appendVariableHeaderField(vector<uint8_t> v, uint32_t num[2]){
    uint32_t res = num[0] | num[1] << ((num[1]*8)+6);
    for (int i = num[1]; i >= 0; i--){
        v.push_back((res >> (8*i)) & 0xff);
    }
    return v;
}
/*
    * parameters:
    *  bytestream to construct message from
    *  size of the bytestream
    * returns:
    *  0 if successful
    *  1 if the packet has problems
    */
uint64_t Message::readByteStream(vector<uint8_t> byteStream){
    if (byteStream.size() < 10){ //do not go out of assigned buffer
        return 1;
    }
    long unsigned int offset = 0;//offset in bytes
    seqNumber[0] = ntohl(transform8to32bit(byteStream, offset));
    seqNumber[1] = 3;
    offset += 4;

    ackNumber[0] = ntohl(transform8to32bit(byteStream, offset));
    ackNumber[1] = 3;
    offset += 4;

    uint32_t* rec = readVariableLengthInteger(byteStream, offset);
    if (*(rec+1) > 3){
        return 1;
    }
    recWindow[0] = *rec;
    recWindow[1] = *(rec+1);
    offset += recWindow[1]+1;
    free(rec);

    uint32_t* next = readVariableLengthInteger(byteStream, offset);
    if (*(next+1) > 3){
        return 1;
    }
    nextHeader[0] = *next;
    nextHeader[1] = *(next+1);
    offset += nextHeader[1]+1;
    free(next);

    headerID nextHeaderID = (headerID) nextHeader[0];
    numAdditionalHeaders = 0;
    while (nextHeaderID != headerID::NO_ADD_HEADER){
        WARN("in loop");
        uint32_t* len;
        uint32_t* curNextHeader;
        if (byteStream.size() < offset){
            return 1;
        }
        if (!((nextHeaderID == headerID::HANDSHAKE_HEADER) || (nextHeaderID == headerID::SECURITY_EXTENSION) || (nextHeaderID == headerID::FIN_RST_HEADER) || (nextHeaderID == headerID::DEBUG))){
            len = readVariableLengthInteger(byteStream, offset);
            if (*(len+1) > 3){
                return 1;
            }
            offset += *len + *(len+1) + 1;

            curNextHeader = readVariableLengthInteger(byteStream, offset);
            if (*(curNextHeader+1) > 3){
                return 1;
            }
            nextHeaderID = (headerID) *curNextHeader;
            offset += *(curNextHeader+1)+1;
        }else{
            len = readVariableLengthInteger(byteStream, offset);
            if (*(len+1) > 3){
                return 1;
            }
            offset += *(len+1)+1;
            if (byteStream.size() < offset + (long unsigned int) *len){
                return 1;
            }
            vector<uint8_t> curExtensionData = {};
            curExtensionData.insert(curExtensionData.begin(), byteStream.begin() + offset, byteStream.begin() + offset + (long unsigned int) *len);
            offset += (long unsigned int) *len;

            curNextHeader = readVariableLengthInteger(byteStream, offset);
            if (*(curNextHeader+1) > 3){
                return 1;
            }
            offset += *(curNextHeader+1)+1;
            GenericHeaderExtension ext = {.headerName = (headerID) nextHeaderID, .length = {*len, *(len+1)}, .extensionData = curExtensionData, .nextHeader = {*curNextHeader, *(curNextHeader+1)}};
            additionalHeaders[numAdditionalHeaders] = ext;
            nextHeaderID = (headerID) *curNextHeader;
            numAdditionalHeaders++;
        }
        free(len);
        free(curNextHeader);
    }
    
    data = {};
    data.insert(data.begin(), byteStream.begin() + offset, byteStream.end());
    WARN("message parsed");
    return 0;
}

uint32_t Message::transform8to32bit(vector<uint8_t> byteStream, uint8_t offset){
    uint32_t a = byteStream[offset];
    uint32_t b = byteStream[offset+1];
    uint32_t c = byteStream[offset+2];
    uint32_t d = byteStream[offset+3];
    return a<<24 | b<<16 | c<<8 | d;
}

//convert next variable length integer to format [content, length-1]
uint32_t* Message::readVariableLengthInteger(vector<uint8_t> byteStream, uint8_t offset){
    // check if vector has sufficient size
    if (byteStream.size() < offset){
        uint32_t* a = (uint32_t*) malloc(sizeof(uint32_t));
        *(a+1) = 0xffffffff;
        return a;
    }
    uint8_t firstBits = byteStream[offset] >> 6;
    // check if vector has sufficient size
    if (byteStream.size() < offset + firstBits){
        uint32_t* a = (uint32_t*) malloc(sizeof(uint32_t));
        *(a+1) = 0xffffffff;
        return a;
    }
    // read the bytes
    uint32_t res = (byteStream[offset] & 0b00111111) << firstBits*8;
    for (int i = 1; i<=firstBits; ++i){
        res = res | (byteStream[i+offset] << (firstBits-i)*8);
    }
    uint32_t* a = (uint32_t*) malloc(2*sizeof(uint32_t));
    *a = res;
    *(a+1) = firstBits;
    return a;
}

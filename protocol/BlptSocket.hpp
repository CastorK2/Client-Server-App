#pragma once
#ifndef SOCKET_H
#define SOCKET_H

#include <vector>
#include <cstdint>
#include <arpa/inet.h>
#include <fmt/core.h>
//#include <fmt/ostream.h>

using namespace std;

#define RECV_BUFFER_SIZE 16384 //vector<uint8_t>().max_size() <- sadly not working
#define SEND_BUFFER_SIZE 16384

class BlptSocket{
public:
    struct sockaddr_in thisAddr;
    socklen_t thisAddrLen;
    struct sockaddr_in otherAddr;
    socklen_t otherAddrLen;
    int udpSocket;
    uint8_t receiveBuffer[RECV_BUFFER_SIZE];
    BlptSocket(sockaddr_in _thisAddr, sockaddr_in _otherAddr, int _udpSocket);
    int send(vector<uint8_t>messageBuffer);
    vector<uint8_t> receive();
};

template<>
struct fmt::formatter<BlptSocket> {
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.end();
    }

    template <typename FormatContext>
    auto format(const BlptSocket& input, FormatContext& ctx) -> decltype(ctx.out()) {
        char otherBuffer[input.otherAddrLen];
        char thisBuffer[input.thisAddrLen];

        inet_ntop(AF_INET, &input.otherAddr, otherBuffer, sizeof(otherBuffer));
        inet_ntop(AF_INET, &input.thisAddr, thisBuffer, sizeof(thisBuffer));
        string otherOut = otherBuffer;
        string thisOut = thisBuffer;
        return format_to(ctx.out(),"(src: {}:{} dst: {}:{})", thisOut, ntohs(input.thisAddr.sin_port), otherOut, ntohs(input.otherAddr.sin_port));
    }
};

#endif //SOCKET_H
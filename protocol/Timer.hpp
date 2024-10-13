#pragma once
#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <ctime>
#include <cmath>

using namespace std;

class Timer{
private:
    std::chrono::time_point<std::chrono::system_clock> m_StartTime;
    std::chrono::time_point<std::chrono::system_clock> m_EndTime;
    bool m_bRunning;
public:
    Timer();
    void start();
    void stop();
    double elapsedMilliseconds();
    bool isRunning();
};

#endif //TIMER_H
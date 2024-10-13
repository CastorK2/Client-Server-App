// https://gist.github.com/mcleary/b0bf4fa88830ff7c882d
#include "Timer.hpp"

using namespace std;

Timer::Timer(){
    m_bRunning = false;
}

void Timer::start(){
    m_StartTime = chrono::system_clock::now();
    m_bRunning = true;
}

void Timer::stop(){
    m_EndTime = chrono::system_clock::now();
    m_bRunning = false;
}

double Timer::elapsedMilliseconds(){
    chrono::time_point<chrono::system_clock> endTime;
    if(m_bRunning){
        endTime = chrono::system_clock::now();
    }else{
        endTime = m_EndTime;
    }
    return chrono::duration_cast<chrono::milliseconds>(endTime - m_StartTime).count();
}

bool Timer::isRunning(){
    return m_bRunning;
}
#pragma once

#include <windows.h>
#include <string>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace ShaiG{
    void getResolution(unsigned &horizontal, unsigned &vertical){
        RECT desktop;
        GetWindowRect(GetDesktopWindow(),&desktop);
        horizontal = desktop.right;
        vertical = desktop.bottom;
    }

    void getResolutionMetric(unsigned &horizontal, unsigned &vertical){
        horizontal = GetSystemMetrics(SM_CXSCREEN);
	    vertical = GetSystemMetrics(SM_CYSCREEN);
    }

    void pauseScreen(bool newLine=1) { 
        std::string temp;
        if (newLine) {
            std::cout << "\n\nPAUSE...";
            std::cin.ignore();
            std::getline(std::cin, temp);
        }
        else {
            std::cout << "\n\nPAUSE...";
            std::getline(std::cin, temp);
        }
    }

    bool durationPassed(
        std::chrono::time_point<std::chrono::high_resolution_clock>& baseTime,
        std::chrono::time_point<std::chrono::high_resolution_clock>& nowTime,
        std::chrono::minutes& duration
    ){
        nowTime = std::chrono::high_resolution_clock::now();
        return (nowTime-baseTime)>=duration;
    }

    class timer{
        std::chrono::time_point<std::chrono::high_resolution_clock> start;
        std::chrono::time_point<std::chrono::high_resolution_clock> end;
        bool started;
        float timeElapsed;

        public:
        void startTimer(){
            start = std::chrono::high_resolution_clock::now();
            started = 1;
        }
        bool endTimer(){
            if(started){
                end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<float>tempDuration{end-start};
                timeElapsed = tempDuration.count(); started = 0;
                return 1;
            }else {timeElapsed = -1;return 0;}
        }
        float getElapsedTime() const{return timeElapsed;}

        timer():started{0},timeElapsed{-1}{}
    };

    void strToArrayW(const std::string& str, wchar_t* wstr) {
        for (int c{ 0 }; c <= str.size(); c++) {
            if(c!= str.size())wstr[c] = str[c];
            else wstr[c] = '\0';
        }
    }

    void strToArrayW(const std::string& str, std::wstring& wstr) {
        wstr.resize(str.size());
        for (int c{ 0 }; c <= str.size(); c++) {
            wstr[c] = str[c];
        }
    }
    
    void getDateTimeStr(std::string& dateTime){
        //Get system time, create filepathname with systime in temp folder and create and open log file
        std::time_t sysTime;
        std::time(&sysTime);
        std::tm cTime;//calendar time
        //std::tm* resultPtr = &result;
        localtime_s(&cTime, &sysTime);//puts sysTime into tm obj which holds time as calendar time
        //places the tm obj with specific format into string buff, stringBuffO;fails if you try to put it directly into wostringstream 
        std::ostringstream stringBuffO;
        stringBuffO << std::put_time(&cTime, "%d.%m.%y_%H.%M.%S");
        dateTime = stringBuffO.str();
    }

    void getDateTimeStrW(std::wstring& dateTime){
        //Get system time, create filepathname with systime in temp folder and create and open log file
        std::time_t sysTime;
        std::time(&sysTime);
        std::tm cTime;//calendar time
        //std::tm* resultPtr = &result;
        localtime_s(&cTime, &sysTime);//puts sysTime into tm obj which holds time as calendar time
        //places the tm obj with specific format into string buff, stringBuffO;fails if you try to put it directly into wostringstream 
        std::ostringstream stringBuffO;
        stringBuffO << std::put_time(&cTime, "%d.%m.%y_%H.%M.%S");
        const std::string dateTimeTemp{stringBuffO.str()};
        strToArrayW(dateTimeTemp, dateTime);
    }
}
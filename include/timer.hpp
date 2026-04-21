#pragma once
#include <chrono>

class Timer{
    public:
        Timer(): total_time(0.0), running(false) {}
        void start(){
            start_time = std::chrono::high_resolution_clock::now();
            running = true;
        }
        void stop(){
            if(running){
                auto end_time = std::chrono::high_resolution_clock::now();
                total_time += std::chrono::duration<double>(end_time - start_time).count();
                running = false;
            }
        }
        double getTotalTime() const {
            return total_time;
        }
        void reset(){
            total_time = 0.0;
            running = false;
        }
    private:
        std::chrono::high_resolution_clock::time_point start_time;
        double total_time;
        bool running;
};

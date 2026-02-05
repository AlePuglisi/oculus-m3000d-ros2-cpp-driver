#ifndef LOGGER_HPP
#define LOGGER_HPP

// Logging System
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;

// Enum to represent log levels
enum LogLevel { DEBUG, INFO, WARNING, ERROR, CRITICAL };

class Logger {
    public: 
        Logger(); 
        ~Logger(); 

        string levelToString(LogLevel level); 

        void log(LogLevel level, const string& message); 
}; 

#endif // LOGGER_HPP
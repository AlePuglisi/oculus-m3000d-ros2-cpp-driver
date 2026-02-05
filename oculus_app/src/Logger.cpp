#include <oculus_app/Logger.hpp>


Logger::Logger()
{
    //std::cout << "Logger Initialized"; 
}

    // Destructor: Closes the log file
Logger::~Logger()
{
    //cout << "Logger Destroyed"; 
}

string Logger::levelToString(LogLevel level)
{
    switch (level) {
    case DEBUG:
        return "DEBUG";
    case INFO:
        return "INFO";
    case WARNING:
        return "WARNING";
    case ERROR:
        return "ERROR";
    case CRITICAL:
        return "CRITICAL";
    default:
        return "UNKNOWN";
    }
}

// Logs a message with a given log level
void Logger::log(LogLevel level, const string& message)
{
    // Get current timestamp
    time_t now = time(0);
    tm* timeinfo = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp),
                "%Y-%m-%d %H:%M:%S", timeinfo);

    // Create log entry
    ostringstream logEntry;
    logEntry << "[" << timestamp << "] "
                << levelToString(level) << ": " << message
                << endl;

    // Output to console
    cout << logEntry.str();
}



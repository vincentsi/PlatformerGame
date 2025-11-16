#include "core/Logger.h"
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

std::unique_ptr<std::ofstream> Logger::logFileStream = nullptr;
bool Logger::initialized = false;

void Logger::init(const std::string& logFile) {
    if (initialized) return;

    if (!logFile.empty()) {
        logFileStream = std::make_unique<std::ofstream>(logFile, std::ios::app);
        if (!logFileStream->is_open()) {
            std::cerr << "Warning: Could not open log file: " << logFile << "\n";
            logFileStream.reset();
        }
    }

    initialized = true;
}

void Logger::shutdown() {
    if (logFileStream && logFileStream->is_open()) {
        logFileStream->close();
    }
    logFileStream.reset();
    initialized = false;
}

std::string Logger::getLevelString(Level level) {
    switch (level) {
        case Debug: return "DEBUG";
        case Info: return "INFO";
        case Warning: return "WARNING";
        case Error: return "ERROR";
        default: return "UNKNOWN";
    }
}

void Logger::log(Level level, const std::string& message) {
    if (!initialized) {
        init();
    }

    auto now = std::time(nullptr);
    std::tm localTime;
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif
    
    std::ostringstream timestamp;
    timestamp << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");

    std::string levelStr = getLevelString(level);
    std::string logMessage = "[" + timestamp.str() + "] [" + levelStr + "] " + message;

    if (level == Error || level == Warning) {
        std::cerr << logMessage << "\n";
    } else {
        std::cout << logMessage << "\n";
    }

    if (logFileStream && logFileStream->is_open()) {
        *logFileStream << logMessage << "\n";
        logFileStream->flush();
    }
}


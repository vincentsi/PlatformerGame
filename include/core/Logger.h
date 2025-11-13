#pragma once

#include <string>
#include <fstream>
#include <memory>

class Logger {
public:
    enum Level {
        Debug,
        Info,
        Warning,
        Error
    };

    // Initialize logger (optional log file)
    static void init(const std::string& logFile = "");

    // Log a message
    static void log(Level level, const std::string& message);

    // Convenience methods
    static void debug(const std::string& message) { log(Debug, message); }
    static void info(const std::string& message) { log(Info, message); }
    static void warning(const std::string& message) { log(Warning, message); }
    static void error(const std::string& message) { log(Error, message); }

    // Shutdown logger
    static void shutdown();

private:
    static std::unique_ptr<std::ofstream> logFileStream;
    static bool initialized;
    static std::string getLevelString(Level level);
};


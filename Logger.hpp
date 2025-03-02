#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include "Globals.hpp"

enum class LogLevel
{
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL,
    PASS
};

enum LogColors
{
    GREEN = 32,
    RED = 31,
    BLUE = 36,
    YELLOW = 93,
    WHITE = 97
};

class Log
{
public:
    std::string log_str;
    LogLevel level;
    bool debug;
    Log(std::string log_str, LogLevel level, bool debug) : log_str(log_str), level(level), debug(debug) {}
};

class Logger
{
public:
    Logger(const char *path = Logger::filename);
    ~Logger();

    static void add_logs(std::string log_str, LogLevel level = LogLevel::INFO, bool debug = true);

private:
    static void createFile();
    static std::mutex queueMutex;
    static std::condition_variable queueConditionVariable;
    static std::queue<Log> loggerQueue;
    static const char *filename;
    static bool stopLogging;

    static std::string format_log(std::string &log_str, LogLevel level);

    std::thread loggerThread;
    std::ofstream file;

    void process_logs();
    void writeLog(const char *content);
};
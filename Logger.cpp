#include "Logger.hpp"

const char *Logger::filename = "./Logs.txt";
bool Logger::stopLogging = false;
std::queue<Log> Logger::loggerQueue;
std::mutex Logger::queueMutex;
std::condition_variable Logger::queueConditionVariable;

Logger::Logger(const char *path)
{
    this->file = std::ofstream(Logger::filename, std::ios::out | std::ios::trunc);
    this->loggerThread = std::thread(&Logger::process_logs, this);
}

Logger::~Logger()
{
    stopLogging = true;
    queueConditionVariable.notify_one();
    loggerThread.join();
    Logger::file.close();
    add_logs("Logger has stopped. Everything that will be on the console won't be stored on the file ", LogLevel::PASS);
}

void Logger::add_logs(std::string log_str, LogLevel level, bool debug)
{

    Log log(log_str, level, debug);
    std::unique_lock<std::mutex> lock(queueMutex);
    if (DEBUG_MODE == false && debug == true)
    {
        return;
    }
    if (stopLogging)
    {
        std::string console_log = format_log(log.log_str, log.level);
        std::cout << console_log;
    }
    else
    {
        loggerQueue.push(log);
        queueConditionVariable.notify_one();
    }
}

void Logger::process_logs()
{
    while (!stopLogging)
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        this->queueConditionVariable.wait(lock, [this]
                                          { return !loggerQueue.empty() || stopLogging; });
        while (!this->loggerQueue.empty())
        {
            Log log = loggerQueue.front();
            std::string console_log = format_log(log.log_str, log.level);
            writeLog(log.log_str.c_str());
            loggerQueue.pop();

            std::cout << console_log;
        }
    }
}

std::string Logger::format_log(std::string &log_str, LogLevel level)
{
    std::string console_log;
    switch (level)
    {
    case LogLevel::DEBUG:
        log_str = "[DEBUG] " + log_str + "\n";
        console_log = log_str;
        break;
    case LogLevel::INFO:
        log_str = "[INFO] " + log_str + "\n";
        console_log = "\033[" + std::to_string(LogColors::BLUE) + "m" + log_str + "\033[" + std::to_string(LogColors::WHITE) + "m";
        break;
    case LogLevel::WARNING:
        log_str = "[WARNING] " + log_str + "\n";
        console_log = "\033[" + std::to_string(LogColors::YELLOW) + "m" + log_str + "\033[" + std::to_string(LogColors::WHITE) + "m";
        break;
    case LogLevel::ERROR:
        log_str = "[ERROR] " + log_str + "\n";
        console_log = "\033[" + std::to_string(LogColors::RED) + "m" + log_str + "\033[" + std::to_string(LogColors::WHITE) + "m";
        break;
    case LogLevel::CRITICAL:
        log_str = "[CRITICAL] " + log_str + "\n";
        console_log = "\033[" + std::to_string(LogColors::RED) + "m" + log_str + "\033[" + std::to_string(LogColors::WHITE) + "m";
        break;
    case LogLevel::PASS:
        log_str = "[PASS] " + log_str + "\n";
        console_log = "\033[" + std::to_string(LogColors::GREEN) + "m" + log_str + "\033[" + std::to_string(LogColors::WHITE) + "m";
        break;
    default:
        break;
    }
    return console_log;
}

void Logger::writeLog(const char *content)
{
    if (!file.is_open())
    {
        exit(0);
    }
    auto end = std::chrono::system_clock::now();
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    file << content << " at " << std::ctime(&end_time) << "\n";
}
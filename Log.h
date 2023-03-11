#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <chrono>

class Log
{
    public:
        static void eraseLogs();
        static void closeFile();
        static void writeLog(const char * content);
    private:
        static void createFile();
        static std::ofstream file;

        static const char * filename;

};
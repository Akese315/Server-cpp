#include "Log.h"

const char * Log::filename = "Logs.txt";
std::ofstream Log::file(Log::filename, std::ios::out|std::ios::trunc);

void Log::writeLog(const char * content)
{  
    if(!file.is_open())
    {
        exit(0);
    }
    auto end = std::chrono::system_clock::now();
    std::time_t end_time = std::chrono::system_clock::to_time_t(end); 
    file << content << " at "<<std::ctime(&end_time)<<"\n";
}

void Log::createFile()
{
    writeLog("Server on  ");
}

void Log::closeFile()
{
    writeLog("Sever off  ");
    Log::file.close();
}
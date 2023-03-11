#pragma once
#include <iostream>
#include "Log.h"
#include <string>

class Console
{
public:
   

    static char *greenColor;
    static char *yellowColor;
    static char *redColor;
    static char *blueColor;
    static char *whiteColor;

    static void printError(std::string error);
    static void printWarning(std::string warning);
    static void printInfo(std::string printInfo);
    static void printSuccess(std::string success);

private:
    
    Log logger;
    
};

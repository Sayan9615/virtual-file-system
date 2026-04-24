#include "logger/ConsoleLogger.h"
#include <iostream>

int main(){
    ConsoleLogger cl;
    cl.log_timed("Nu a putut fi inregistrat");

    return 0;
}
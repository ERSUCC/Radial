#pragma once

#include <iostream>
#include <string>

#include "exception.h"

struct Process
{
    static int run(std::string cmd, const bool primary, const bool display = true);
};

#pragma once

#include <string>

#include "exception.h"
#include "utils.h"

struct Process
{
    static int run(std::string cmd, const bool primary, const bool display = true);
};

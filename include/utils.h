#pragma once

#include <iostream>
#include <stddef.h>
#include <string>
#include <string.h>

struct Utils
{
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);

    static size_t numericVersion(const std::string& version);
};

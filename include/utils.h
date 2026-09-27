#pragma once

#include <iostream>
#include <stddef.h>
#include <string>
#include <string.h>
#include <vector>

#include "exception.h"

struct Utils
{
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);

    static size_t numericVersion(const std::string& version);

    static std::string trim(const std::string& str);
    static std::string escapeQuotes(const std::string& str);
    static std::string ensureSuffix(const std::string& str, const std::string& suffix);

    static std::vector<std::string> split(const std::string& str, const std::string& sep);

private:
    static void printPrefixed(const std::string& message, const std::string& prefix);

};

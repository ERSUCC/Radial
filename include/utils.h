#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stddef.h>
#include <string>
#include <string.h>

#include "exception.h"

struct Utils
{
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);

    static size_t numericVersion(const std::string& version);

    static std::string readFile(const std::filesystem::path& path);
    static std::string trim(const std::string& str);
    static std::string escapeQuotes(const std::string& str);

private:
    static void printPrefixed(const std::string& message, const std::string& prefix);

};

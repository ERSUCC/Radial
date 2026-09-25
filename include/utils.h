#pragma once

#include <filesystem>
#include <fstream>
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

    static std::string readString(const std::filesystem::path& path);
    static std::vector<std::string> readLines(const std::filesystem::path& path);

    static std::string trim(const std::string& str);
    static std::string escapeQuotes(const std::string& str);
    static std::string ensureSuffix(const std::string& str, const std::string& suffix);

    static bool endsWith(const std::string& str, const std::string& suffix);

    static std::vector<std::string> split(const std::string& str, const std::string& sep);

    static void writeString(const std::filesystem::path& path, const std::string& data);

private:
    static void printPrefixed(const std::string& message, const std::string& prefix);

};

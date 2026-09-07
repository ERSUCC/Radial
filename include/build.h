#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdio.h>

#include "config.h"
#include "exception.h"
#include "options.h"
#include "utils.h"

struct Build
{
    static void run(const BuildOptions* options);
    static void build(BuildEnvironment& env);

private:
    static void compile(BuildEnvironment& env, const std::filesystem::path& file);
    static void link(BuildEnvironment& env, const std::filesystem::path& file);
    static void runProc(const std::string& cmd);

};

struct Cache
{
    static std::filesystem::file_time_type readTime(const BuildEnvironment& env, const std::filesystem::path& file);

    static void writeTime(const BuildEnvironment& env, const std::filesystem::path& file);

private:
    static std::filesystem::path cachePath(const BuildEnvironment& env, const std::filesystem::path& file);

};

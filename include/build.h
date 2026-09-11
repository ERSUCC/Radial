#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

#include "config.h"
#include "exception.h"
#include "options.h"
#include "process.h"
#include "utils.h"

#ifdef _WIN32

#define OBJ_EXT ".obj"

#else

#define OBJ_EXT ".o"

#endif

struct Build
{
    static void run(const BuildOptions* options);
    static void build(BuildEnvironment& env);

private:
    static void compile(BuildEnvironment& env, const std::filesystem::path& file);
    static void link(BuildEnvironment& env, const std::filesystem::path& file);

    static std::string compileCommand(const BuildEnvironment& env, const std::filesystem::path& file, const std::filesystem::path& object);
    static std::string linkCommand(const BuildEnvironment& env, const std::filesystem::path& file);

};

struct Cache
{
    static std::filesystem::file_time_type readTime(const BuildEnvironment& env, const std::filesystem::path& file);

    static void writeTime(const BuildEnvironment& env, const std::filesystem::path& file);

private:
    static std::filesystem::path cachePath(const BuildEnvironment& env, const std::filesystem::path& file);

};

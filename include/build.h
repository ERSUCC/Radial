#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <utility>

#include "config.h"
#include "exception.h"
#include "options.h"
#include "process.h"
#include "utils.h"

#ifdef _WIN32

#define OBJ_EXT ".obj"
#define BIN_EXT ".exe"

#else

#define OBJ_EXT ".o"
#define BIN_EXT ""

#endif

struct Build
{
    static void run(const BuildOptions* options);
    static void build(BuildEnvironment& env, const bool force);

private:
    static void compile(BuildEnvironment& env, const std::filesystem::path& file, const bool force);
    static void link(BuildEnvironment& env, const std::filesystem::path& file);

    static std::string compileCommand(const BuildEnvironment& env, const std::filesystem::path& file, const std::filesystem::path& object);
    static std::string linkCommand(const BuildEnvironment& env, const std::filesystem::path& file);

    static bool shouldUpdate(const BuildEnvironment& env, const std::filesystem::path& file, const std::filesystem::path& object);

};

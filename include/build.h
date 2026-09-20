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
    static void build(const BuildOptions* options, BuildEnvironment& env);

private:
    static void compile(const BuildOptions* options, BuildEnvironment& env, const std::filesystem::path& file);
    static void link(const BuildOptions* options, BuildEnvironment& env, const std::filesystem::path& file);

    static std::string compileCommand(const BuildOptions* options, const BuildEnvironment& env, const std::filesystem::path& file, const std::filesystem::path& object);
    static std::string linkCommand(const BuildOptions* options, const BuildEnvironment& env, const std::filesystem::path& file);

    static bool shouldUpdate(const BuildEnvironment& env, const std::filesystem::path& file, const std::filesystem::path& object);

};

#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <unordered_set>

#include "exception.h"
#include "options.h"
#include "toml.h"
#include "utils.h"

typedef std::unordered_set<std::filesystem::path> PathSet;

struct BuildEnvironment
{
    BuildEnvironment(const std::string& name, const int stdVersion, const std::filesystem::path& dest);

    const std::string name;

    const int stdVersion;

    const std::filesystem::path dest;

    PathSet includes;
    PathSet sources;
    PathSet objects;
};

struct Build
{
    static void run(const BuildOptions* options);

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

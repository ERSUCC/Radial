#pragma once

#include <filesystem>
#include <iostream>
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

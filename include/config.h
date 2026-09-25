#pragma once

#include <filesystem>
#include <memory>
#include <stddef.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "exception.h"
#include "options.h"
#include "process.h"
#include "toml.h"
#include "utils.h"

typedef std::unordered_set<std::filesystem::path> PathSet;

struct Config
{
    static std::unique_ptr<const TOML> readConfig(const std::filesystem::path& root);
};

struct BuildEnvironment
{
    static BuildEnvironment create(const BuildOptions* options);

    const BuildOptions* options;

    std::unique_ptr<const TOML> config;

    const std::string name;

    const int stdVersion;

    const std::filesystem::path dest;
    const std::filesystem::path cache;

    std::string compilerPath;
    std::string linkerPath;

    PathSet includeDirs;
    PathSet libDirs;
    PathSet sources;
    PathSet objects;

    std::unordered_map<std::filesystem::path, PathSet> includes;

    std::unordered_set<std::string> libs;

    ListMap<std::string, std::string> defines;

private:
    BuildEnvironment(const BuildOptions* options, std::unique_ptr<const TOML> config, const std::string& name, const int stdVersion, const std::filesystem::path& dest);

    static std::filesystem::path homePath();

    static void findCompiler(BuildEnvironment& env);
    static void findIncludes(const BuildEnvironment& env, const std::filesystem::path& path, PathSet& includes);

};

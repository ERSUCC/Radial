#pragma once

#include <filesystem>
#include <memory>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <unordered_set>
#include <utility>
#include <vector>

#include "exception.h"
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
    static BuildEnvironment create(const std::filesystem::path& root);

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

private:
    BuildEnvironment(std::unique_ptr<const TOML> config, const std::string& name, const int stdVersion, const std::filesystem::path& dest);

    static void findCompiler(BuildEnvironment& env);

};

#pragma once

#include <filesystem>
#include <memory>
#include <optional>
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
typedef std::unordered_map<std::filesystem::path, PathSet> PathMap;

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

    const std::filesystem::path root;
    const std::filesystem::path dest;
    const std::filesystem::path cache;

    std::string compilerPath;
    std::string linkerPath;

    PathSet includeDirs;
    PathSet includeDirsLocal;
    PathSet libDirs;
    PathSet sources;
    PathSet objects;

    PathMap includes;

    std::unordered_set<std::string> libs;

    ListMap<std::string, std::string> defines;

private:
    BuildEnvironment(const BuildOptions* options, std::unique_ptr<const TOML> config, const std::string& name, const int stdVersion, const std::filesystem::path& root, const std::filesystem::path& dest);

    static std::filesystem::path homePath();

    static void findCompiler(BuildEnvironment& env);
    static void findIncludes(const BuildEnvironment& env, const std::filesystem::path& path, PathSet& includes, PathMap& visited);

    static std::optional<std::string> includeName(const std::string& str);

};

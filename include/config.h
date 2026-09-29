#pragma once

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
#include "path.h"
#include "process.h"
#include "source.h"
#include "toml.h"
#include "utils.h"

#ifdef _WIN32

#define OS_KEY "windows"

#elif __APPLE__

#define OS_KEY "mac"

#else

#define OS_KEY "linux"

#endif

struct Config
{
    static std::unique_ptr<const TOML> readConfig(const Path& root);
};

struct BuildEnvironment
{
    static BuildEnvironment create(const BuildOptions* options);

    const BuildOptions* options;

    std::unique_ptr<const TOML> config;

    const std::string name;

    const int stdVersion;

    const Path root;
    const Path dest;
    const Path cache;

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
    BuildEnvironment(const BuildOptions* options, std::unique_ptr<const TOML> config, const std::string& name, const int stdVersion, const Path& root, const Path& dest);

    static Path homePath();

    static void findCompiler(BuildEnvironment& env);
    static void findIncludes(const BuildEnvironment& env, const Path& path, PathSet& includes, PathMap& visited);

    static std::optional<std::string> includeName(const std::string& str);

};

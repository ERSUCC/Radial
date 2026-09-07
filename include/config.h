#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "exception.h"
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

    PathSet includes;
    PathSet sources;
    PathSet objects;

private:
    BuildEnvironment(std::unique_ptr<const TOML> config, const std::string& name, const int stdVersion, const std::filesystem::path& dest);

};

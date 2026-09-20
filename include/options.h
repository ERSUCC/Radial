#pragma once

#include <optional>
#include <string>
#include <string.h>

#include "exception.h"

enum struct CommandType
{
    Build,
    Run
};

struct CommandOptions
{
    CommandOptions(const CommandType& command);

    virtual ~CommandOptions();

    const CommandType command;
};

struct BuildOptions : public CommandOptions
{
    BuildOptions(const CommandType& type = CommandType::Build);

    bool force = false;
    bool debug = false;

    std::optional<std::string> root;
};

struct RunOptions : public BuildOptions
{
    RunOptions();
};

struct ProgramOptions
{
    static ProgramOptions parse(char** argv, const int argc);

    bool version = false;

    std::optional<CommandOptions*> commandOptions;

private:
    static BuildOptions* parseBuild(char** argv, const int argc);
    static RunOptions* parseRun(char** argv, const int argc);

    static std::optional<std::string> getFlag(char* arg);

};

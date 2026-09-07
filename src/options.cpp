#include "options.h"

CommandOptions::CommandOptions(const CommandType& command) :
    command(command) {}

CommandOptions::~CommandOptions() {}

BuildOptions::BuildOptions() :
    CommandOptions(CommandType::Build) {}

ProgramOptions ProgramOptions::parse(char** argv, const int argc)
{
    if (argc < 1)
    {
        throw RadialUsageException("");
    }

    ProgramOptions options;

    int current = 0;

    while (current < argc)
    {
        if (const std::optional<std::string> flag = getFlag(argv[current]))
        {
            if (flag == "help")
            {
                throw RadialUsageException("");
            }

            if (flag == "version")
            {
                options.version = true;
            }

            else
            {
                throw RadialArgumentException("Unknown option \"" + flag.value() + "\". Use `radial --help` for information about program options.");
            }

            current++;
        }

        else
        {
            break;
        }
    }

    if (current < argc)
    {
        if (!strncmp(argv[current], "build", 6))
        {
            options.commandOptions = parseBuild(argv + 1, argc - current - 1);
        }

        else
        {
            throw RadialArgumentException("Unknown command \"" + std::string(argv[current]) + "\". Use `radial --help` for information about available commands.");
        }
    }

    return options;
}

BuildOptions* ProgramOptions::parseBuild(char** argv, const int argc)
{
    BuildOptions* options = new BuildOptions();

    int current = 0;

    while (current < argc)
    {
        if (const std::optional<std::string> flag = getFlag(argv[current]))
        {
            if (flag == "help")
            {
                throw RadialUsageException("build");
            }

            throw RadialArgumentException("Unknown option \"" + flag.value() + "\" for command \"build\". Use `radial build --help` for information about command options.");
        }

        else
        {
            break;
        }
    }

    if (argc - current > 1)
    {
        throw RadialArgumentException("Command \"build\" only accepts one positional argument. See `radial build --help` for usage instructions.");
    }

    if (current < argc)
    {
        options->root = argv[current];
    }

    return options;
}

std::optional<std::string> ProgramOptions::getFlag(char* arg)
{
    if (!strncmp(arg, "--", 2))
    {
        return arg + 2;
    }

    if (!strncmp(arg, "-", 1))
    {
        return arg + 1;
    }

    return std::nullopt;
}

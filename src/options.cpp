#include "options.h"

CommandOptions::CommandOptions(const CommandType& command) :
    command(command) {}

CommandOptions::~CommandOptions() {}

BuildOptions::BuildOptions(const CommandType& type) :
    CommandOptions(type) {}

RunOptions::RunOptions() :
    BuildOptions(CommandType::Run) {}

CleanOptions::CleanOptions() :
    CommandOptions(CommandType::Clean) {}

ProgramOptions ProgramOptions::parse(char** argv, const int argc)
{
    if (argc < 1)
    {
        throw RadialUsageException("", 1);
    }

    ProgramOptions options;

    int current = 0;

    while (current < argc)
    {
        if (const std::optional<std::string> flag = getFlag(argv[current]))
        {
            if (flag == "help")
            {
                throw RadialUsageException("", 0);
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

        else if (!strncmp(argv[current], "run", 4))
        {
            options.commandOptions = parseRun(argv + 1, argc - current - 1);
        }

        else if (!strncmp(argv[current], "clean", 6))
        {
            options.commandOptions = parseClean(argv + 1, argc - current - 1);
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
                throw RadialUsageException("build", 0);
            }

            if (flag == "force")
            {
                options->force = true;
            }

            else if (flag == "debug")
            {
                options->debug = true;
            }

            else
            {
                throw RadialArgumentException("Unknown option \"" + flag.value() + "\" for command \"build\". Use `radial build --help` for information about command options.");
            }

            current++;
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

RunOptions* ProgramOptions::parseRun(char** argv, const int argc)
{
    RunOptions* options = new RunOptions();

    int current = 0;

    while (current < argc)
    {
        if (const std::optional<std::string> flag = getFlag(argv[current]))
        {
            if (flag.value().empty())
            {
                break;
            }

            if (flag == "help")
            {
                throw RadialUsageException("run", 0);
            }

            if (flag == "force")
            {
                options->force = true;
            }

            else if (flag == "debug")
            {
                options->debug = true;
            }

            else
            {
                throw RadialArgumentException("Unknown option \"" + flag.value() + "\" for command \"run\". Use `radial run --help` for information about command options.");
            }

            current++;
        }

        else
        {
            break;
        }
    }

    if (current < argc && strncmp(argv[current], "--", 3))
    {
        options->root = argv[current++];
    }

    if (current < argc && !strncmp(argv[current], "--", 3))
    {
        current++;
    }

    while (current < argc)
    {
        options->args.push_back(argv[current++]);
    }

    return options;
}

CleanOptions* ProgramOptions::parseClean(char** argv, const int argc)
{
    CleanOptions* options = new CleanOptions();

    int current = 0;

    while (current < argc)
    {
        if (const std::optional<std::string> flag = getFlag(argv[current]))
        {
            if (flag == "help")
            {
                throw RadialUsageException("clean", 0);
            }

            throw RadialArgumentException("Unknown option \"" + flag.value() + "\" for command \"clean\". Use `radial clean --help` for information about command options.");

            current++;
        }

        else
        {
            break;
        }
    }

    if (argc - current > 1)
    {
        throw RadialArgumentException("Command \"clean\" only accepts one positional argument. See `radial clean --help` for usage instructions.");
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

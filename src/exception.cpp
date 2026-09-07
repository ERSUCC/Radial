#include "exception.h"

RadialException::RadialException(const std::string& message) :
    message(message) {}

const char* RadialException::what() const noexcept
{
    return message.c_str();
}

RadialUsageException::RadialUsageException(const std::string& command) :
    RadialException(getUsageMessage(command)) {}

std::string RadialUsageException::getUsageMessage(const std::string& command)
{
    std::string message;

    if (command.empty())
    {
        message += "Usage: radial [--version] [--help] <command> [<args>...]\n\n";
        message += "Commands:\n";
        message += "  build\n\n";
        message += "Use `radial <command> --help` for information about a specific command.";
    }

    else if (command == "build")
    {
        message += "Usage: radial build [--help] [<path>]\n\n";
        message += "Build the project in the specified directory. If no path is specified, build the\n";
        message += "project in the current directory.\n\n";
        message += "Use `radial --help` for information about other commands.";
    }

    return message;
}

RadialArgumentException::RadialArgumentException(const std::string& message) :
    RadialException(message) {}

RadialFileException::RadialFileException(const std::string& message) :
    RadialException(message) {}

RadialConfigException::RadialConfigException(const std::string& message) :
    RadialException(message) {}

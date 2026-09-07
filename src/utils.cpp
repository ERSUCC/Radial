#include "utils.h"

void Utils::info(const std::string& message)
{
    std::cout << "[info] " << message << "\n";
}

void Utils::warning(const std::string& message)
{
    std::cout << "[warning] " << message << "\n";
}

void Utils::error(const std::string& message)
{
    std::cout << "[error] " << message << "\n";
}

size_t Utils::numericVersion(const std::string& version)
{
    char* data = (char*)malloc(sizeof(char) * (version.size() + 1));

    strncpy(data, version.c_str(), version.size());

    const size_t major = atoll(strtok(data, "."));
    const size_t minor = atoll(strtok(nullptr, "."));
    const size_t patch = atoll(strtok(nullptr, "."));

    free(data);

    return major * 65536 + minor * 256 + patch;
}

#include "utils.h"

void Utils::info(const std::string& message)
{
    printPrefixed(message, "[info] ");
}

void Utils::warning(const std::string& message)
{
    printPrefixed(message, "[warning] ");
}

void Utils::error(const std::string& message)
{
    printPrefixed(message, "[error] ");
}

size_t Utils::numericVersion(const std::string& version)
{
    char* data = (char*)malloc(sizeof(char) * (version.size() + 1));

    strncpy(data, version.c_str(), version.size() + 1);

    const size_t major = atoll(strtok(data, "."));
    const size_t minor = atoll(strtok(nullptr, "."));
    const size_t patch = atoll(strtok(nullptr, "."));

    free(data);

    return major * 65536 + minor * 256 + patch;
}

std::string Utils::readFile(const std::filesystem::path& path)
{
    std::ifstream file(path);

    if (!file.is_open())
    {
        throw RadialFileException("Failed to read file: \"" + path.string() + "\"");
    }

    std::string data;

    while (!file.eof())
    {
        char buffer[1025];

        file.read(buffer, 1024);

        if (file.bad())
        {
            throw RadialFileException("Failed to read file: \"" + path.string() + "\"");
        }

        buffer[file.gcount()] = '\0';

        data += buffer;
    }

    file.close();

    return data;
}

std::string Utils::trim(const std::string& str)
{
    char* data = (char*)malloc(sizeof(char) * (str.size() + 1));

    strncpy(data, str.c_str(), str.size() + 1);

    size_t start = 0;

    while (start < str.size() && isspace(str[start]))
    {
        start++;
    }

    size_t end = str.size() - 1;

    while (end > start && isspace(str[end]))
    {
        end--;
    }

    if (end <= start)
    {
        return "";
    }

    const size_t length = end - start + 1;

    char* trimmed = (char*)malloc(sizeof(char) * (length + 1));

    strncpy(trimmed, data + start, length);

    trimmed[length] = '\0';

    free(data);

    return trimmed;
}

void Utils::printPrefixed(const std::string& message, const std::string& prefix)
{
    char* data = (char*)malloc(sizeof(char) * (message.size() + 1));

    strncpy(data, message.c_str(), message.size() + 1);

    std::cout << prefix << strtok(data, "\n") << "\n";

    while (const char* token = strtok(nullptr, "\n"))
    {
        std::cout << prefix << token << "\n";
    }

    free(data);
}

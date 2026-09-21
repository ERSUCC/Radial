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

std::string Utils::readString(const std::filesystem::path& path)
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

std::vector<std::string> Utils::readLines(const std::filesystem::path& path)
{
    std::ifstream file(path);

    if (!file.is_open())
    {
        throw RadialFileException("Failed to read file: \"" + path.string() + "\"");
    }

    std::vector<std::string> lines;

    std::string line;

    while (!file.eof())
    {
        char buffer[1025];

        file.read(buffer, 1024);

        if (file.bad())
        {
            throw RadialFileException("Failed to read file: \"" + path.string() + "\"");
        }

        for (size_t i = 0; i < file.gcount(); i++)
        {
            if (buffer[i] == '\n')
            {
                lines.push_back(line);

                line = "";
            }

            else
            {
                line += buffer[i];
            }
        }
    }

    file.close();

    return lines;
}

void Utils::writeString(const std::filesystem::path& path, const std::string& data)
{
    std::ofstream file(path);

    if (!file.is_open())
    {
        throw RadialFileException("Failed to open file: \"" + path.string() + "\"");
    }

    file << data;

    file.close();
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

std::string Utils::escapeQuotes(const std::string& str)
{
    std::string escaped;

    for (const char c : str)
    {
        if (c == '"')
        {
            escaped += "\\\"";
        }

        else
        {
            escaped += c;
        }
    }

    return escaped;
}

void Utils::printPrefixed(const std::string& message, const std::string& prefix)
{
    std::string line;

    for (size_t i = 0; i < message.size(); i++)
    {
        if (message[i] == '\n')
        {
            std::cout << prefix << line << '\n';

            line = "";
        }

        else
        {
            line += message[i];
        }
    }

    if (!line.empty())
    {
        std::cout << prefix << line << '\n';
    }
}

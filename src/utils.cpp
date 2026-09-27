#include "utils.h"

void Utils::info(const std::string& message)
{
    printPrefixed(message, "[info] ");
}

void Utils::warning(const std::string& message)
{
    std::cout << "\x1b[38;2;210;210;50m";

    printPrefixed(message, "[warning] ");

    std::cout << "\x1b[49;;m";
}

void Utils::error(const std::string& message)
{
    std::cout << "\x1b[38;2;255;128;128m";

    printPrefixed(message, "[error] ");

    std::cout << "\x1b[49;;m";
}

size_t Utils::numericVersion(const std::string& version)
{
    const std::vector<std::string> sections = split(version, ".");

    return atoll(sections[0].c_str()) * 65536 + atoll(sections[1].c_str()) * 256 + atoll(sections[2].c_str());
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

    if (end < start)
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

std::string Utils::ensureSuffix(const std::string& str, const std::string& suffix)
{
    if (str.size() < suffix.size())
    {
        return str + suffix;
    }

    for (size_t i = 1; i <= suffix.size(); i++)
    {
        if (str[str.size() - i] != suffix[suffix.size() - i])
        {
            return str + suffix;
        }
    }

    return str;
}

std::vector<std::string> Utils::split(const std::string& str, const std::string& sep)
{
    char* data = (char*)malloc(sizeof(char) * (str.size() + 1));

    strncpy(data, str.c_str(), str.size() + 1);

    char* token = data;

    strtok(token, sep.c_str());

    std::vector<std::string> items;

    do
    {
        items.push_back(token);
    } while (token = strtok(nullptr, sep.c_str()));

    free(data);

    return items;
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

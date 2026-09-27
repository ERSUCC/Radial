#include "path.h"

Path Path::absolute(const std::filesystem::path& path)
{
    return std::filesystem::weakly_canonical(path);
}

Path Path::temp(const std::filesystem::path& path)
{
    return std::filesystem::temp_directory_path() / path;
}

Path::Path(const std::filesystem::path& path) :
    path(path) {}

Path::Path(const std::string& path) :
    path(path) {}

Path::Path(const char* path) :
    path(path) {}

std::vector<Path> Path::children(const bool recursive) const
{
    if (!isDirectory())
    {
        return {};
    }

    std::vector<Path> paths;

    if (recursive)
    {
        for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(path))
        {
            if (!entry.is_directory())
            {
                paths.push_back(entry.path());
            }
        }
    }

    else
    {
        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(path))
        {
            if (!entry.is_directory())
            {
                paths.push_back(entry.path());
            }
        }
    }

    return paths;
}

std::string Path::read() const
{
    std::ifstream file(path);

    if (!file.is_open())
    {
        throw RadialFileException("Failed to read file: \"" + string() + "\"");
    }

    std::string data;

    while (!file.eof())
    {
        char buffer[1025];

        file.read(buffer, 1024);

        if (file.bad())
        {
            throw RadialFileException("Failed to read file: \"" + string() + "\"");
        }

        buffer[file.gcount()] = '\0';

        data += buffer;
    }

    file.close();

    return data;
}

std::vector<std::string> Path::readLines() const
{
    std::ifstream file(path);

    if (!file.is_open())
    {
        throw RadialFileException("Failed to read file: \"" + string() + "\"");
    }

    std::vector<std::string> lines;

    std::string line;

    while (!file.eof())
    {
        char buffer[1025];

        file.read(buffer, 1024);

        if (file.bad())
        {
            throw RadialFileException("Failed to read file: \"" + string() + "\"");
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

void Path::write(const std::string& str) const
{
    std::ofstream file(path);

    if (!file.is_open())
    {
        throw RadialFileException("Failed to open file: \"" + string() + "\"");
    }

    file << str;

    file.close();
}

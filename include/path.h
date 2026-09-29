#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "exception.h"

struct Path
{
    friend struct PathHash;
    friend struct PathEquals;

    static Path absolute(const std::filesystem::path& path);
    static Path temp(const std::filesystem::path& path);

    Path(const std::filesystem::path& path);
    Path(const std::string& path);
    Path(const char* path);

    inline bool exists() const
    {
        return std::filesystem::exists(path);
    }

    inline bool isFile() const
    {
        return std::filesystem::is_regular_file(path);
    }

    inline bool isDirectory() const
    {
        return std::filesystem::is_directory(path);
    }

    inline bool isAbsolute() const
    {
        return path.is_absolute();
    }

    inline bool contains(const Path& path) const
    {
        return std::filesystem::relative(path.path, this->path).string()[0] != '.';
    }

    inline std::string name() const
    {
        return path.filename().string();
    }

    inline std::string extension() const
    {
        return path.extension().string();
    }

    inline std::string string() const
    {
        return path.string();
    }

    inline Path parent() const
    {
        return path.parent_path();
    }

    inline Path absolute() const
    {
        return Path::absolute(path);
    }

    inline std::filesystem::file_time_type lastWrite() const
    {
        return std::filesystem::last_write_time(path);
    }

    friend Path operator/(const Path& left, const Path& right)
    {
        return left.path / right.path;
    }

    inline void remove() const
    {
        std::filesystem::remove_all(path);
    }

    inline void createDirectories() const
    {
        std::filesystem::create_directories(path);
    }

    std::vector<Path> children(const bool recursive) const;

    std::string read() const;
    std::vector<std::string> readLines() const;

    void write(const std::string& str) const;

private:
    const std::filesystem::path path;

};

struct PathHash
{
    std::size_t operator()(const Path& path) const
    {
        return std::filesystem::hash_value(path.path);
    }
};

struct PathEquals
{
    bool operator()(const Path& a, const Path& b) const
    {
        return a.path == b.path;
    }
};

typedef std::unordered_set<Path, PathHash, PathEquals> PathSet;
typedef std::unordered_map<Path, PathSet, PathHash, PathEquals> PathMap;

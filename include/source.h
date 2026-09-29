#pragma once

#include <sstream>
#include <stddef.h>

struct SourceLocation
{
    SourceLocation(const size_t line, const size_t character);

    const size_t line;
    const size_t character;
};

struct Source
{
    Source(const std::string& str);

    inline char peek()
    {
        return stream.peek();
    }

    inline bool eof() const
    {
        return stream.eof();
    }

    inline SourceLocation location() const
    {
        return SourceLocation(currentLine, currentCharacter);
    }

    char get();

    std::string read(const size_t length);

    void skipWhitespace(const bool multiline = false);

private:
    std::istringstream stream;

    size_t currentLine = 1;
    size_t currentCharacter = 1;

};

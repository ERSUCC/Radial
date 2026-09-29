#include "source.h"

SourceLocation::SourceLocation(const size_t line, const size_t character) :
    line(line), character(character) {}

Source::Source(const std::string& str) :
    stream(str) {}

char Source::get()
{
    const char c = stream.get();

    if (c == '\n')
    {
        currentLine++;
        currentCharacter = 1;
    }

    else if (c != std::istringstream::traits_type::eof())
    {
        currentCharacter++;
    }

    return c;
}

std::string Source::read(const size_t length)
{
    std::string str;

    for (size_t i = 0; i < length; i++)
    {
        if (stream.eof())
        {
            return str;
        }

        str += get();
    }

    return str;
}

void Source::skipWhitespace(const bool multiline)
{
    while (peek() == '#' || peek() == ' ' || peek() == '\t' || (multiline && peek() == '\n'))
    {
        if (peek() == '#')
        {
            while (!eof() && peek() != '\n')
            {
                get();
            }
        }

        else
        {
            get();
        }
    }
}

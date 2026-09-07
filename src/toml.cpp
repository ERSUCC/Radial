#include "toml.h"

void TOMLUtils::skipWhitespace(std::istringstream& stream, const bool multiline)
{
    while (stream.peek() == ' ' || stream.peek() == '\t' || (multiline && stream.peek() == '\n'))
    {
        stream.ignore();
    }
}

TOMLValue* TOMLValue::parse(std::istringstream& stream)
{
    TOMLUtils::skipWhitespace(stream);

    const char c = stream.peek();

    if (c == '[')
    {
        stream.ignore();

        return TOMLArray::parse(stream);
    }

    if (c == '\'')
    {
        stream.ignore();

        return TOMLString::parse(stream, true);
    }

    if (c == '"')
    {
        stream.ignore();

        return TOMLString::parse(stream, false);
    }

    if (c == '-' || c == '+' || isdigit(c))
    {
        return TOMLInteger::parse(stream);
    }

    try
    {
        return TOMLBoolean::parse(stream);
    }

    catch (const RadialException& ex)
    {
        throw RadialConfigException("Unexpected character \"" + std::string(1, c) + "\".");
    }
}

TOMLValue* TOMLValue::getDefault()
{
    static TOMLValue* defaultValue;

    if (!defaultValue)
    {
        defaultValue = new TOMLValue();
    }

    return defaultValue;
}

TOMLValue::~TOMLValue() {}

Option<std::vector<const TOMLValue*>> TOMLValue::array() const
{
    return {};
}

Option<std::string> TOMLValue::string() const
{
    return {};
}

Option<int> TOMLValue::integer() const
{
    return {};
}

Option<bool> TOMLValue::boolean() const
{
    return {};
}

TOMLArray* TOMLArray::parse(std::istringstream& stream)
{
    TOMLArray* array = new TOMLArray();

    while (stream.peek() != ']')
    {
        TOMLUtils::skipWhitespace(stream, true);

        try
        {
            array->values.push_back(TOMLValue::parse(stream));
        }

        catch (const RadialException& ex)
        {
            delete array;

            throw;
        }

        TOMLUtils::skipWhitespace(stream, true);

        if (stream.peek() == ',')
        {
            stream.ignore();
        }
    }

    if (stream.peek() != ']')
    {
        delete array;

        throw RadialConfigException("Expected \"]\", but received \"" + std::string(1, stream.peek()) + "\".");
    }

    stream.ignore();

    return array;
}

TOMLArray::~TOMLArray()
{
    for (const TOMLValue* value : values)
    {
        delete value;
    }
}

Option<std::vector<const TOMLValue*>> TOMLArray::array() const
{
    return values;
}

TOMLString* TOMLString::parse(std::istringstream& stream, const bool literal)
{
    std::string value;

    if (literal)
    {
        while (!stream.eof() && stream.peek() != '\'')
        {
            value += stream.get();
        }
    }

    else
    {
        while (!stream.eof() && stream.peek() != '"')
        {
            const char c = stream.get();

            if (c == '\\')
            {
                const char escape = stream.get();

                switch (escape)
                {
                    case 'b':
                        value += '\b';

                        break;

                    case 't':
                        value += '\t';

                        break;

                    case 'n':
                        value += '\n';

                        break;

                    case 'f':
                        value += '\f';

                        break;

                    case 'r':
                        value += '\r';

                        break;

                    case 'e':
                        value += '\e';

                        break;

                    case '"':
                        value += '"';

                        break;

                    case '\\':
                        value += '\\';

                        break;

                    default:
                        throw RadialConfigException("Unrecognized escape sequence \"\\" + std::string(1, escape) + "\"");
                }
            }

            else
            {
                value += c;
            }
        }
    }

    if (stream.eof())
    {
        throw RadialConfigException("Unexpected end of file.");
    }

    stream.ignore();

    return new TOMLString(value);
}

Option<std::string> TOMLString::string() const
{
    return value;
}

TOMLString::TOMLString(const std::string& value) :
    value(value) {}

TOMLInteger* TOMLInteger::parse(std::istringstream& stream)
{
    std::string value;

    do
    {
        value += stream.get();
    } while (isdigit(stream.peek()));

    return new TOMLInteger(atoi(value.c_str()));
}

Option<int> TOMLInteger::integer() const
{
    return value;
}

TOMLInteger::TOMLInteger(const int value) :
    value(value) {}

TOMLBoolean* TOMLBoolean::parse(std::istringstream& stream)
{
    char data[4];

    stream.read(data, 4);

    if (!strncmp(data, "true", 4))
    {
        return new TOMLBoolean(true);
    }

    if (!strncmp(data, "fals", 4) && stream.get() == 'e')
    {
        return new TOMLBoolean(false);
    }

    throw RadialConfigException("Invalid configuration.");
}

Option<bool> TOMLBoolean::boolean() const
{
    return value;
}

TOMLBoolean::TOMLBoolean(const bool value) :
    value(value) {}

TOMLEntry* TOMLEntry::parse(std::istringstream& stream)
{
    const std::string key = TOMLEntry::parseKey(stream);

    TOMLUtils::skipWhitespace(stream);

    if (stream.peek() != '=')
    {
        throw RadialConfigException("Expected \"=\", but received \"" + std::string(1, stream.peek()) + "\".");
    }

    stream.ignore();

    return new TOMLEntry(key, TOMLValue::parse(stream));
}

TOMLEntry::~TOMLEntry()
{
    delete value;
}

TOMLEntry::TOMLEntry(const std::string& key, const TOMLValue* value) :
    key(key), value(value) {}

std::string TOMLEntry::parseKey(std::istringstream& stream)
{
    TOMLUtils::skipWhitespace(stream);

    if (!keyChar(stream.peek()))
    {
        throw RadialConfigException("Invalid key character \"" + std::string(1, stream.peek()) + "\".");
    }

    std::string key;

    do
    {
        key += stream.get();
    } while (keyChar(stream.peek()));

    return key;
}

bool TOMLEntry::keyChar(const char c)
{
    return isalnum(c) || c == '_' || c == '-';
}

TOML* TOML::parse(std::istringstream& stream)
{
    TOML* toml = new TOML();

    while (!stream.eof())
    {
        while (stream.peek() == '\n')
        {
            stream.ignore();
        }

        if (stream.eof())
        {
            break;
        }

        try
        {
            toml->entries.push_back(TOMLEntry::parse(stream));
        }

        catch (const RadialException& ex)
        {
            delete toml;

            throw;
        }

        TOMLUtils::skipWhitespace(stream);

        if (stream.peek() != '\n')
        {
            delete toml;

            throw RadialConfigException("Expected newline, but received \"" + std::string(1, stream.peek()) + "\".");
        }
    }

    return toml;
}

TOML* TOML::parse(const std::filesystem::path& path)
{
    std::ifstream file(path);

    if (!file.is_open())
    {
        throw RadialFileException("Failed to open file " + path.string() + ".");
    }

    std::string data;

    while (!file.eof())
    {
        char buffer[1025];

        file.read(buffer, 1024);

        if (file.bad())
        {
            throw RadialFileException("Failed to read file " + path.string() + ".");
        }

        buffer[file.gcount()] = '\0';

        data += buffer;
    }

    file.close();

    std::istringstream stream(data);

    return TOML::parse(stream);
}

TOML::~TOML()
{
    for (const TOMLEntry* entry : entries)
    {
        delete entry;
    }
}

const TOMLValue* TOML::get(const std::string& key) const
{
    for (const TOMLEntry* entry : entries)
    {
        if (entry->key == key)
        {
            return entry->value;
        }
    }

    return TOMLValue::getDefault();
}

#include "toml.h"

RadialTokenException::RadialTokenException(const std::string& expected, std::istringstream& stream) :
    RadialConfigException(getMessage(expected, stream)) {}

std::string RadialTokenException::getMessage(const std::string& expected, std::istringstream& stream)
{
    const std::string prefix = "Expected " + expected + ", but received ";

    if (stream.eof())
    {
        return prefix + "end of file.";
    }

    const char c = stream.peek();

    if (c == '\n')
    {
        return prefix + "newline.";
    }

    return prefix + "\"" + std::string(1, c) + "\".";
}

void TOMLUtils::skipWhitespace(std::istringstream& stream, const bool multiline)
{
    while (stream.peek() == '#' || stream.peek() == ' ' || stream.peek() == '\t' || (multiline && stream.peek() == '\n'))
    {
        if (stream.peek() == '#')
        {
            while (!stream.eof() && stream.peek() != '\n')
            {
                stream.ignore();
            }
        }

        else
        {
            stream.ignore();
        }
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

    return TOMLBoolean::parse(stream);
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

Option<ListMap<std::string, const TOMLValue*>> TOMLValue::table() const
{
    return {};
}

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

void TOMLValue::write(std::ostringstream& stream) const {}

TOMLTable* TOMLTable::parse(std::istringstream& stream)
{
    TOMLTable* table = new TOMLTable({});

    while (!stream.eof())
    {
        TOMLUtils::skipWhitespace(stream, true);

        if (stream.eof() || stream.peek() == '[')
        {
            break;
        }

        try
        {
            const TOMLEntry* entry = TOMLEntry::parse(stream);

            table->entries.add(entry->key, entry);
        }

        catch (const RadialConfigException& ex)
        {
            delete table;

            throw;
        }

        TOMLUtils::skipWhitespace(stream);

        if (stream.peek() != '\n')
        {
            delete table;

            throw RadialTokenException("newline", stream);
        }
    }

    return table;
}

TOMLTable::TOMLTable(const std::vector<const TOMLEntry*>& entries)
{
    for (const TOMLEntry* entry : entries)
    {
        this->entries.add(entry->key, entry);
    }
}

TOMLTable::~TOMLTable()
{
    for (const std::string& key : entries.keys())
    {
        delete entries.get(key);
    }
}

Option<ListMap<std::string, const TOMLValue*>> TOMLTable::table() const
{
    ListMap<std::string, const TOMLValue*> values;

    for (const std::string& key : entries.keys())
    {
        values.add(key, entries.get(key)->value);
    }

    return values;
}

void TOMLTable::write(std::ostringstream& stream) const
{
    for (const std::string& key : entries.keys())
    {
        entries.get(key)->write(stream);
    }
}

TOMLArray* TOMLArray::parse(std::istringstream& stream)
{
    TOMLArray* array = new TOMLArray({});

    while (stream.peek() != ']')
    {
        TOMLUtils::skipWhitespace(stream, true);

        if (stream.peek() == ']')
        {
            break;
        }

        try
        {
            array->values.push_back(TOMLValue::parse(stream));
        }

        catch (const RadialConfigException& ex)
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

        throw RadialTokenException("\"]\"", stream);
    }

    stream.ignore();

    return array;
}

TOMLArray::TOMLArray(const std::vector<const TOMLValue*>& values) :
    values(values) {}

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

void TOMLArray::write(std::ostringstream& stream) const
{
    if (values.empty())
    {
        stream << "[]";

        return;
    }

    stream << "[\n  ";

    values[0]->write(stream);

    for (size_t i = 1; i < values.size(); i++)
    {
        stream << ",\n  ";

        values[i]->write(stream);
    }

    stream << "\n]";
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
        throw RadialTokenException("closing quotation mark", stream);
    }

    stream.ignore();

    return new TOMLString(value, literal);
}

TOMLString::TOMLString(const std::string& value, const bool literal) :
    value(value), literal(literal) {}

Option<std::string> TOMLString::string() const
{
    return value;
}

void TOMLString::write(std::ostringstream& stream) const
{
    if (literal)
    {
        stream << '\'' << value << '\'';
    }

    else
    {
        stream << '"';

        for (const char c : value)
        {
            switch (c)
            {
                case '\b':
                    stream << "\\b";

                    break;

                case '\t':
                    stream << "\\t";

                    break;

                case '\n':
                    stream << "\\n";

                    break;

                case '\f':
                    stream << "\\f";

                    break;

                case '\r':
                    stream << "\\r";

                    break;

                case '\"':
                    stream << "\\\"";

                    break;

                case '\\':
                    stream << "\\\\";

                    break;

                default:
                    stream << c;

                    break;
            }
        }

        stream << '"';
    }
}

TOMLInteger* TOMLInteger::parse(std::istringstream& stream)
{
    std::string value;

    do
    {
        value += stream.get();
    } while (isdigit(stream.peek()));

    return new TOMLInteger(atoi(value.c_str()));
}

TOMLInteger::TOMLInteger(const int value) :
    value(value) {}

Option<int> TOMLInteger::integer() const
{
    return value;
}

void TOMLInteger::write(std::ostringstream& stream) const
{
    stream << value << '\n';
}

TOMLBoolean::TOMLBoolean(const bool value) :
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

    throw RadialConfigException("Expected value.");
}

Option<bool> TOMLBoolean::boolean() const
{
    return value;
}

void TOMLBoolean::write(std::ostringstream& stream) const
{
    if (value)
    {
        stream << "true";
    }

    else
    {
        stream << "false";
    }
}

TOMLEntry* TOMLEntry::parse(std::istringstream& stream)
{
    if (stream.peek() == '[')
    {
        stream.ignore();

        TOMLUtils::skipWhitespace(stream);

        const std::string key = parseKey(stream);

        TOMLUtils::skipWhitespace(stream);

        if (stream.peek() != ']')
        {
            throw RadialTokenException("\"]\"", stream);
        }

        stream.ignore();

        return new TOMLEntry(key, TOMLTable::parse(stream), true);
    }

    const std::string key = parseKey(stream);

    TOMLUtils::skipWhitespace(stream);

    if (stream.peek() != '=')
    {
        throw RadialTokenException("\"=\"", stream);
    }

    stream.ignore();

    return new TOMLEntry(key, TOMLValue::parse(stream));
}

TOMLEntry::TOMLEntry(const std::string& key, const TOMLValue* value, const bool table) :
    key(key), value(value), table(table) {}

TOMLEntry::~TOMLEntry()
{
    delete value;
}

void TOMLEntry::write(std::ostringstream& stream) const
{
    if (table)
    {
        stream << "\n[" << key << "]\n";

        value->write(stream);
    }

    else
    {
        stream << key << " = ";

        value->write(stream);

        stream << '\n';
    }
}

std::string TOMLEntry::parseKey(std::istringstream& stream)
{
    if (!keyChar(stream.peek()))
    {
        throw RadialTokenException("key character", stream);
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
    TOML* toml = new TOML({});

    while (!stream.eof())
    {
        TOMLUtils::skipWhitespace(stream, true);

        if (stream.eof())
        {
            break;
        }

        try
        {
            const TOMLEntry* entry = TOMLEntry::parse(stream);

            toml->entries.add(entry->key, entry);
        }

        catch (const RadialConfigException& ex)
        {
            delete toml;

            throw;
        }

        TOMLUtils::skipWhitespace(stream);

        if (!stream.eof() && stream.peek() != '\n')
        {
            delete toml;

            throw RadialTokenException("newline", stream);
        }
    }

    return toml;
}

TOML* TOML::parse(const std::filesystem::path& path)
{
    std::istringstream stream(Utils::readString(path));

    return TOML::parse(stream);
}

TOML::TOML(const std::vector<const TOMLEntry*>& entries)
{
    for (const TOMLEntry* entry : entries)
    {
        this->entries.add(entry->key, entry);
    }
}

TOML::~TOML()
{
    for (const std::string& key : entries.keys())
    {
        delete entries.get(key);
    }
}

const TOMLValue* TOML::get(const std::string& key) const
{
    if (entries.contains(key))
    {
        return entries.get(key)->value;
    }

    return TOMLValue::getDefault();
}

void TOML::write(std::ostringstream& stream) const
{
    for (const std::string& key : entries.keys())
    {
        entries.get(key)->write(stream);
    }
}

void TOML::write(const std::filesystem::path& path) const
{
    std::ostringstream stream;

    write(stream);

    Utils::writeString(path, stream.str());
}

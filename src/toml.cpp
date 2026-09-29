#include "toml.h"

RadialTokenException::RadialTokenException(const std::string& expected, Source& source) :
    RadialConfigException(getMessage(expected, source), source.location()) {}

std::string RadialTokenException::getMessage(const std::string& expected, Source& source)
{
    const std::string prefix = "Expected " + expected + ", but received ";

    if (source.eof())
    {
        return prefix + "end of file.";
    }

    const char c = source.peek();

    if (c == '\n')
    {
        return prefix + "newline.";
    }

    return prefix + "\"" + std::string(1, c) + "\".";
}

TOMLValue* TOMLValue::parse(Source& source)
{
    source.skipWhitespace();

    const char c = source.peek();

    if (c == '[')
    {
        return TOMLArray::parse(source);
    }

    if (c == '\'')
    {
        return TOMLString::parse(source, true);
    }

    if (c == '"')
    {
        return TOMLString::parse(source, false);
    }

    if (c == '-' || c == '+' || isdigit(c))
    {
        return TOMLInteger::parse(source);
    }

    return TOMLBoolean::parse(source);
}

TOMLValue* TOMLValue::getDefault()
{
    static TOMLValue* defaultValue;

    if (!defaultValue)
    {
        defaultValue = new TOMLValue(SourceLocation(0, 0));
    }

    return defaultValue;
}

TOMLValue::TOMLValue(const SourceLocation& location) :
    location(location) {}

TOMLValue::~TOMLValue() {}

Option<ListMap<std::string, const TOMLValue*>> TOMLValue::table() const
{
    return Option<ListMap<std::string, const TOMLValue*>>(location);
}

Option<std::vector<const TOMLValue*>> TOMLValue::array() const
{
    return Option<std::vector<const TOMLValue*>>(location);
}

Option<std::string> TOMLValue::string() const
{
    return Option<std::string>(location);
}

Option<int> TOMLValue::integer() const
{
    return Option<int>(location);
}

Option<bool> TOMLValue::boolean() const
{
    return Option<bool>(location);
}

void TOMLValue::write(std::ostringstream& stream) const {}

TOMLTable* TOMLTable::parse(Source& source)
{
    TOMLTable* table = new TOMLTable(source.location(), {});

    while (!source.eof())
    {
        source.skipWhitespace(true);

        if (source.eof() || source.peek() == '[')
        {
            break;
        }

        try
        {
            const TOMLEntry* entry = TOMLEntry::parse(source);

            table->entries.add(entry->key, entry);
        }

        catch (const RadialConfigException& ex)
        {
            delete table;

            throw;
        }

        source.skipWhitespace();

        if (source.peek() != '\n')
        {
            delete table;

            throw RadialTokenException("newline", source);
        }
    }

    return table;
}

TOMLTable::TOMLTable(const SourceLocation& location, const std::vector<const TOMLEntry*>& entries) :
    TOMLValue(location)
{
    for (const TOMLEntry* entry : entries)
    {
        this->entries.add(entry->key, entry);
    }
}

TOMLTable::TOMLTable(const std::vector<const TOMLEntry*>& entries) :
    TOMLTable(SourceLocation(0, 0), entries) {}

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

    return Option<ListMap<std::string, const TOMLValue*>>(location, values);
}

void TOMLTable::write(std::ostringstream& stream) const
{
    for (const std::string& key : entries.keys())
    {
        entries.get(key)->write(stream);
    }
}

TOMLArray* TOMLArray::parse(Source& source)
{
    TOMLArray* array = new TOMLArray(source.location(), {});

    source.get();

    while (source.peek() != ']')
    {
        source.skipWhitespace(true);

        if (source.peek() == ']')
        {
            break;
        }

        try
        {
            array->values.push_back(TOMLValue::parse(source));
        }

        catch (const RadialConfigException& ex)
        {
            delete array;

            throw;
        }

        source.skipWhitespace(true);

        if (source.peek() == ',')
        {
            source.get();
        }
    }

    if (source.peek() != ']')
    {
        delete array;

        throw RadialTokenException("\"]\"", source);
    }

    source.get();

    return array;
}

TOMLArray::TOMLArray(const SourceLocation& location, const std::vector<const TOMLValue*>& values) :
    TOMLValue(location), values(values) {}

TOMLArray::TOMLArray(const std::vector<const TOMLValue*>& values) :
    TOMLArray(SourceLocation(0, 0), values) {}

TOMLArray::~TOMLArray()
{
    for (const TOMLValue* value : values)
    {
        delete value;
    }
}

Option<std::vector<const TOMLValue*>> TOMLArray::array() const
{
    return Option<std::vector<const TOMLValue*>>(location, values);
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

TOMLString* TOMLString::parse(Source& source, const bool literal)
{
    const SourceLocation location = source.location();

    source.get();

    std::string value;

    if (literal)
    {
        while (!source.eof() && source.peek() != '\'')
        {
            value += source.get();
        }
    }

    else
    {
        while (!source.eof() && source.peek() != '"')
        {
            const char c = source.get();

            if (c == '\\')
            {
                const SourceLocation location = source.location();

                const char escape = source.get();

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
                        throw RadialConfigException("Unrecognized escape sequence \"\\" + std::string(1, escape) + "\"", location);
                }
            }

            else
            {
                value += c;
            }
        }
    }

    if (source.eof())
    {
        throw RadialTokenException("closing quotation mark", source);
    }

    source.get();

    return new TOMLString(location, value, literal);
}

TOMLString::TOMLString(const SourceLocation& location, const std::string& value, const bool literal) :
    TOMLValue(location), value(value), literal(literal) {}

TOMLString::TOMLString(const std::string& value, const bool literal) :
    TOMLString(SourceLocation(0, 0), value, literal) {}

Option<std::string> TOMLString::string() const
{
    return Option<std::string>(location, value);
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

TOMLInteger* TOMLInteger::parse(Source& source)
{
    const SourceLocation location = source.location();

    std::string value;

    do
    {
        value += source.get();
    } while (isdigit(source.peek()));

    return new TOMLInteger(location, atoi(value.c_str()));
}

TOMLInteger::TOMLInteger(const SourceLocation& location, const int value) :
    TOMLValue(location), value(value) {}

TOMLInteger::TOMLInteger(const int value) :
    TOMLInteger(SourceLocation(0, 0), value) {}

Option<int> TOMLInteger::integer() const
{
    return Option<int>(location, value);
}

void TOMLInteger::write(std::ostringstream& stream) const
{
    stream << value << '\n';
}

TOMLBoolean::TOMLBoolean(const SourceLocation& location, const bool value) :
    TOMLValue(location), value(value) {}

TOMLBoolean::TOMLBoolean(const bool value) :
    TOMLBoolean(SourceLocation(0, 0), value) {}

TOMLBoolean* TOMLBoolean::parse(Source& source)
{
    const SourceLocation location = source.location();

    const std::string str = source.read(4);

    if (str == "true")
    {
        return new TOMLBoolean(location, true);
    }

    if (str == "fals" && source.get() == 'e')
    {
        return new TOMLBoolean(location, false);
    }

    throw RadialConfigException("Expected value.", location);
}

Option<bool> TOMLBoolean::boolean() const
{
    return Option<bool>(location, value);
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

TOMLEntry* TOMLEntry::parse(Source& source)
{
    if (source.peek() == '[')
    {
        source.get();

        source.skipWhitespace();

        const std::string key = parseKey(source);

        source.skipWhitespace();

        if (source.peek() != ']')
        {
            throw RadialTokenException("\"]\"", source);
        }

        source.get();

        return new TOMLEntry(key, TOMLTable::parse(source), true);
    }

    const std::string key = parseKey(source);

    source.skipWhitespace();

    if (source.peek() != '=')
    {
        throw RadialTokenException("\"=\"", source);
    }

    source.get();

    return new TOMLEntry(key, TOMLValue::parse(source));
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

std::string TOMLEntry::parseKey(Source& source)
{
    if (!keyChar(source.peek()))
    {
        throw RadialTokenException("key character", source);
    }

    std::string key;

    do
    {
        key += source.get();
    } while (keyChar(source.peek()));

    return key;
}

bool TOMLEntry::keyChar(const char c)
{
    return isalnum(c) || c == '_' || c == '-';
}

TOML* TOML::parse(Source& source)
{
    TOML* toml = new TOML({});

    while (!source.eof())
    {
        source.skipWhitespace(true);

        if (source.eof())
        {
            break;
        }

        try
        {
            const TOMLEntry* entry = TOMLEntry::parse(source);

            toml->entries.add(entry->key, entry);
        }

        catch (const RadialConfigException& ex)
        {
            delete toml;

            throw;
        }

        source.skipWhitespace();

        if (!source.eof() && source.peek() != '\n')
        {
            delete toml;

            throw RadialTokenException("newline", source);
        }
    }

    return toml;
}

TOML* TOML::parse(const Path& path)
{
    Source source(path.read());

    return TOML::parse(source);
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

void TOML::write(const Path& path) const
{
    std::ostringstream stream;

    write(stream);

    path.write(stream.str());
}

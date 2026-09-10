#pragma once

#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <string.h>
#include <vector>

#include "exception.h"
#include "utils.h"

template <typename T> struct Option
{
    Option(T value) :
        value(value), exists(true) {}

    Option() {}

    T get(T defaultValue)
    {
        if (exists)
        {
            return value;
        }

        return defaultValue;
    }

    T require(const std::string& message)
    {
        if (exists)
        {
            return value;
        }

        throw RadialConfigException(message);
    }

private:
    T value;

    const bool exists = false;

};

struct TOMLUtils
{
    static void skipWhitespace(std::istringstream& stream, const bool multiline = false);
};

struct TOMLValue
{
    static TOMLValue* parse(std::istringstream& stream);
    static TOMLValue* getDefault();

    virtual ~TOMLValue();

    virtual Option<std::vector<const TOMLValue*>> array() const;

    virtual Option<std::string> string() const;

    virtual Option<int> integer() const;

    virtual Option<bool> boolean() const;

private:
    static TOMLValue* defaultValue;

};

struct TOMLArray : public TOMLValue
{
    static TOMLArray* parse(std::istringstream& stream);

    ~TOMLArray();

    Option<std::vector<const TOMLValue*>> array() const override;

private:
    std::vector<const TOMLValue*> values;

};

struct TOMLString : public TOMLValue
{
    static TOMLString* parse(std::istringstream& stream, const bool literal);

    Option<std::string> string() const override;

private:
    TOMLString(const std::string& value);

    const std::string value;

};

struct TOMLInteger : public TOMLValue
{
    static TOMLInteger* parse(std::istringstream& stream);

    Option<int> integer() const override;

private:
    TOMLInteger(const int value);

    const int value;

};

struct TOMLBoolean : public TOMLValue
{
    static TOMLBoolean* parse(std::istringstream& stream);

    Option<bool> boolean() const override;

private:
    TOMLBoolean(const bool value);

    const bool value;

};

struct TOMLEntry
{
    static TOMLEntry* parse(std::istringstream& stream);

    ~TOMLEntry();

    const std::string key;

    const TOMLValue* value;

private:
    TOMLEntry(const std::string& key, const TOMLValue* value);

    static std::string parseKey(std::istringstream& stream);

    static bool keyChar(const char c);

};

struct TOML
{
    static TOML* parse(std::istringstream& stream);
    static TOML* parse(const std::filesystem::path& path);

    ~TOML();

    const TOMLValue* get(const std::string& key) const;

private:
    std::vector<const TOMLEntry*> entries;

};

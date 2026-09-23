#pragma once

#include <filesystem>
#include <sstream>
#include <string>
#include <string.h>
#include <unordered_map>
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

template <typename K, typename V> struct ListMap
{
    void add(const K& key, V value)
    {
        map[key] = value;

        order.push_back(key);
    }

    inline bool contains(const K& key) const
    {
        return map.count(key);
    }

    inline V get(const K& key) const
    {
        return map.at(key);
    }

    inline size_t size() const
    {
        return map.size();
    }

    inline bool empty() const
    {
        return map.empty();
    }

    inline const std::vector<K>& keys() const
    {
        return order;
    }

private:
    std::unordered_map<K, V> map;
    std::vector<K> order;

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

    virtual Option<ListMap<std::string, const TOMLValue*>> table() const;
    virtual Option<std::vector<const TOMLValue*>> array() const;
    virtual Option<std::string> string() const;
    virtual Option<int> integer() const;
    virtual Option<bool> boolean() const;

    virtual void write(std::ostringstream& stream) const;

private:
    static TOMLValue* defaultValue;

};

struct TOMLEntry;

struct TOMLTable : public TOMLValue
{
    static TOMLTable* parse(std::istringstream& stream);

    TOMLTable(const std::vector<const TOMLEntry*>& entries);
    ~TOMLTable();

    Option<ListMap<std::string, const TOMLValue*>> table() const override;

    void write(std::ostringstream& stream) const override;

private:
    ListMap<std::string, const TOMLEntry*> entries;

};

struct TOMLArray : public TOMLValue
{
    static TOMLArray* parse(std::istringstream& stream);

    TOMLArray(const std::vector<const TOMLValue*>& values);
    ~TOMLArray();

    Option<std::vector<const TOMLValue*>> array() const override;

    void write(std::ostringstream& stream) const override;

private:
    std::vector<const TOMLValue*> values;

};

struct TOMLString : public TOMLValue
{
    static TOMLString* parse(std::istringstream& stream, const bool literal);

    TOMLString(const std::string& value, const bool literal);

    Option<std::string> string() const override;

    void write(std::ostringstream& stream) const override;

private:
    const std::string value;

    const bool literal;

};

struct TOMLInteger : public TOMLValue
{
    static TOMLInteger* parse(std::istringstream& stream);

    TOMLInteger(const int value);

    Option<int> integer() const override;

    void write(std::ostringstream& stream) const override;

private:
    const int value;

};

struct TOMLBoolean : public TOMLValue
{
    static TOMLBoolean* parse(std::istringstream& stream);

    TOMLBoolean(const bool value);

    Option<bool> boolean() const override;

    void write(std::ostringstream& stream) const override;

private:
    const bool value;

};

struct TOMLEntry
{
    static TOMLEntry* parse(std::istringstream& stream);

    TOMLEntry(const std::string& key, const TOMLValue* value, const bool table = false);
    ~TOMLEntry();

    void write(std::ostringstream& stream) const;

    const std::string key;

    const TOMLValue* value;

private:
    static std::string parseKey(std::istringstream& stream);

    static bool keyChar(const char c);

    const bool table;

};

struct TOML
{
    static TOML* parse(std::istringstream& stream);
    static TOML* parse(const std::filesystem::path& path);

    TOML(const std::vector<const TOMLEntry*>& entries);
    ~TOML();

    const TOMLValue* get(const std::string& key) const;

    void write(std::ostringstream& stream) const;
    void write(const std::filesystem::path& path) const;

private:
    ListMap<std::string, const TOMLEntry*> entries;

};

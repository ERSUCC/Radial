#pragma once

#include <exception>
#include <string>

struct RadialException : public std::exception
{
    RadialException(const std::string& message);

    const char* what() const noexcept override;

private:
    const std::string message;

};

struct RadialUsageException : public RadialException
{
    RadialUsageException(const std::string& command);

private:
    static std::string getUsageMessage(const std::string& command);

};

struct RadialArgumentException : public RadialException
{
    RadialArgumentException(const std::string& message);
};

struct RadialFileException : public RadialException
{
    RadialFileException(const std::string& message);
};

struct RadialConfigException : public RadialException
{
    RadialConfigException(const std::string& message);
};

#pragma once

#include <vector>
#include <string>
#include <tuple>
#include <map>

#include "Argument.hpp"

class ArgParser
{
public:
    ArgParser(const std::string&& program_name);

public:
    Argument& AddArgument(const char short_name, const std::string&& long_name) noexcept;

    void SetDescription(const std::string&& description) noexcept;
    void SetEpilog(const std::string&& epilog) noexcept;

    std::tuple<bool, std::string> ParseArgument(int& argc, char* argv[]) noexcept;

    std::string operator[](const std::string&& name);
    operator std::string() noexcept;

private:
    const std::string& program_name_;

    std::vector<Argument> arguments_;
    std::map<Argument*, std::string> values_;

    std::string description_;
    std::string epilog_;
};
#pragma once

#include <string>

class Argument {
public:
    Argument(const char short_name, const std::string&& long_name);

public:
    Argument& required();
    Argument& help(const std::string&& help);
    Argument& default_value(const std::string&& default_value);
    Argument& metavar(const std::string&& metavar);
    Argument& flag();

public:
    const std::string& get_longname() noexcept;
    const char& get_shortname() noexcept;
    const std::string& get_help() noexcept;
    const std::string& get_default_value() noexcept;
    const std::string& get_metavar() noexcept;
    const bool& is_flag() noexcept;

private:
    const char short_name_;
    const std::string long_name_;

    bool required_;
    bool is_flag_;
    std::string help_;
    std::string default_value_;
    std::string metavar_;
};

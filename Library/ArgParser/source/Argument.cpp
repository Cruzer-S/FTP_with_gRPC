#include "Argument.hpp"

Argument::Argument(const std::string&& short_name, const std::string&& long_name)
    : short_name_(short_name)
    , long_name_(long_name)
{
    required_ = false;
    is_flag_ = false;
}

Argument& Argument::required()
{
    required_ = true;
}

Argument& Argument::help(const std::string&& help)
{
    help_ = help;
}

Argument& Argument::default_value(const std::string&& default_value)
{
    default_value_ = default_value;
}

Argument& Argument::metavar(const std::string&& metavar) noexcept
{
    metavar_ = metavar;
}

Argument& Argument::flag()
{
    is_flag_ = true;
}

const std::string& Argument::get_longname() noexcept
{
    return long_name_;
}

const char& Argument::get_shortname() noexcept
{
    return short_name_;
}

const std::string& Argument::get_help() noexcept
{
    return help_;
}

const std::string& Argument::get_default_value() noexcept
{
    return default_value_;
}

const std::string& Argument::get_metavar() noexcept
{
    return metavar_;
}

const bool& Argument::is_flag() noexcept
{
    return is_flag_;
}
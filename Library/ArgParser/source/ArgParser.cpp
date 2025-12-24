#include "ArgParser.hpp"

#include <getopt.h>

ArgParser::ArgParser(const std::string&& program_name)
    : program_name_(program_name)
{
}

void ArgParser::SetDescription(const std::string&& description) noexcept
{
    description_ = description;
}

void ArgParser::SetEpilog(const std::string&& epilog) noexcept
{
    epilog_ = epilog;
}

Argument& ArgParser::AddArgument(const char short_name, const std::string&& long_name) noexcept
{
    arguments_.emplace_back(short_name, std::move(long_name));

    return arguments_.back();
}

std::tuple<bool, std::string> ArgParser::ParseArgument(int& argc, char* argv[]) noexcept
{
    struct option options[arguments_.size() + 1];
    std::string shrtopt = "";

    for (int i = 0; i < arguments_.size(); i++) {
        options[i].name = arguments_[i].get_longname().c_str();
        options[i].has_arg = arguments_[i].is_flag() ? no_argument : required_argument;
        options[i].flag = nullptr;
        options[i].val = arguments_[i].get_shortname();

        shrtopt += options[i].val;
        if (options[i].has_arg == required_argument)
            shrtopt += ":";
    }
    options[arguments_.size()] = { nullptr, 0, nullptr, 0 };

    int ch, idx;
    while ((ch = getopt_long(argc, argv, shrtopt.c_str(), options, &idx)) != -1) {
        if (ch == '?') // unknown option
            break;
        if (ch == ':') // missing argument
            break;

        values_[&arguments_[idx]] = optarg;
    }

    argc -= optind;
    argv += optind;

    return { false, "Not implemented yet" };
}

std::string ArgParser::operator[](const std::string&& name)
{
    return "Not implemented yet";
}

ArgParser::operator std::string() noexcept
{
    return "Not implemented yet";
}
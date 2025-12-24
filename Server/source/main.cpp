#include <iostream>
#include <variant>
#include <string>
#include <map>

#include <getopt.h>

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>

#include "FTPServiceImpl.hpp"

using ArgList = std::map<std::string, std::string>;

std::variant<ArgList, std::string> ParseArgument(int argc, char* argv[])
{
    ArgList arglist;

    const struct option options[] = {
            { "loglevel", required_argument, nullptr, 'l' },
            { "root-dir", required_argument, nullptr, 'r' },
            { nullptr, 0, nullptr, 0 }
    };

    try {
        int optidx;
        for (int opt; (opt = getopt_long(argc, argv, "l:r:", options, &optidx)) != -1; ) {
            switch (opt) {
            case 'l':
                arglist["loglevel"] = optarg;
                break;
            case 'r':
                arglist["root-dir"] = optarg;
                break;
            case ':':
                return fmt::format("missing argument: {}", static_cast<char>(opt));
            case '?':
                return fmt::format("invalid argument: {}", static_cast<char>(opt));
            }
        }
    }
    catch (std::exception& e) {
        return fmt::format("invalid argument: {}", e.what());
    }

    argc -= optind;
    if (argc < 2)
        return fmt::format("usage: {} [--loglevel <level>] [--root-dir <directory>] <host> <service>", *argv);

    argv += optind;

    arglist["host"] = *argv++;
    arglist["service"] = *argv++;

    if (arglist.find("root-dir") == arglist.end())
        arglist["root-dir"] = ".";

    if (arglist.find("loglevel") == arglist.end())
        arglist["loglevel"] = "0";

    return arglist;
}

void ShowArgument(const ArgList& arglist)
{
    spdlog::info(fmt::format("Host: {}\tService: {}", arglist.at("host"), arglist.at("service")));
    if (arglist.find("loglevel") != arglist.end())
        spdlog::info(fmt::format("Log Level: {}", arglist.at("loglevel")));
    if (arglist.find("root-dir") != arglist.end())
        spdlog::info(fmt::format("Root Directory: {}", arglist.at("root-dir")));
}

int main(int argc, char* argv[])
{
    const auto out = ParseArgument(argc, argv);
    if (const auto failed_to_parse = std::get_if<std::string>(&out)) {
        spdlog::error("failed to ParseArgument(): {}", *failed_to_parse);
        return 1;
    }

    const ArgList& arglist = std::get<ArgList>(out);
    ShowArgument(arglist);

    FTPServiceImpl service(arglist.at("root-dir"));
    if (!service.IsValid()) {
        spdlog::error("failed to create FTP Service: invalid root directory {}", arglist.at("root-dir"));
        return 1;
    }
    spdlog::info("registered service(s): FTP");

    grpc::ServerBuilder builder;
    builder.AddListeningPort(fmt::format("{}:{}", arglist.at("host"), arglist.at("service")), grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    spdlog::info("server started: listening on {}:{}", arglist.at("host"), arglist.at("service"));

    (void)getchar();

    server->Shutdown();

    spdlog::info("server stopped", arglist.at("host"), arglist.at("service"));

    return 0;
}
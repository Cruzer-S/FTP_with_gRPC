
#include <memory>
#include <variant>
#include <string>
#include <map>

#include <getopt.h>

#include <grpcpp/support/server_interceptor.h>
#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "FTPServiceImpl.hpp"
#include "ServerInterceptor.hpp"

using ArgList = std::map<std::string, std::string>;

std::pair<bool, std::variant<ArgList, std::string>> ParseArgument(int argc, char* argv[])
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
                return { false, fmt::format("missing argument: {}", static_cast<char>(opt)) };
            case '?':
                return { false, fmt::format("invalid argument: {}", static_cast<char>(opt)) };
            }
        }
    }
    catch (std::exception& e) {
        return { false, fmt::format("invalid argument: {}", e.what()) };
    }

    argc -= optind;
    if (argc < 2)
        return { false, fmt::format("usage: {} [--loglevel <level>] [--root-dir <directory>] <host> <service>", *argv) };

    argv += optind;

    arglist["host"] = *argv++;
    arglist["service"] = *argv++;

    if (arglist.find("root-dir") == arglist.end())
        arglist["root-dir"] = ".";

    if (arglist.find("loglevel") == arglist.end())
        arglist["loglevel"] = "0";

    return { true, arglist };
}

void ShowArgument(const ArgList& arglist)
{
    for (const auto &[name, value]: arglist)
	    spdlog::info("{}: {}", name, value);
}

int main(int argc, char* argv[])
{
    const auto &[success, result] = ParseArgument(argc, argv);
    if (!success) {
        spdlog::error("failed to ParseArgument(): {}", std::get<std::string>(result));
        return 1;
    }

    const ArgList& arglist = std::get<ArgList>(result);
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

    server->Wait();

    server->Shutdown();

    spdlog::info("server stopped", arglist.at("host"), arglist.at("service"));

    return 0;
}

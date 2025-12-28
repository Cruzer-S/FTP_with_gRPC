#include <variant>
#include <string>

#include <getopt.h>

#include <google/protobuf/util/time_util.h>
#include <google/protobuf/timestamp.pb.h>
#include <grpcpp/grpcpp.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

#include "FTPClient.hpp"

using ArgList = std::map<std::string, std::string>;

std::pair<bool, std::variant<ArgList, std::string>> ParseArgument(int argc, char* argv[])
{
        ArgList arglist;

        const struct option options[] = {
                { nullptr, 0, nullptr, 0 }
        };

        try {
                int optidx;
                for (int opt; (opt = getopt_long(argc, argv, "", options, &optidx)) != -1; ) {
                        switch (opt) {
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
        if (argc < 4)
                return { false, fmt::format("usage: {} <host> <service> <infile> <outpath>", *argv) };

        argv += optind;

        arglist["host"] = *argv++;
        arglist["service"] = *argv++;
        arglist["infile"] = *argv++;
        arglist["outpath"] = *argv++;

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

        const std::string target = fmt::format("{}:{}", arglist.at("host"), arglist.at("service"));
        std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
        spdlog::info("channel opened at: {}", target);

        FTPClient client(channel);

        const auto [success_upload, metadata, status] = client.UploadFile(arglist.at("infile"), arglist.at("outpath"));
        if (success_upload) {
                spdlog::error("failed to upload file: {}", status.message);
                return 1;
        }

        spdlog::info("file uploaded successfully: \n{}", metadata.DebugString());

        return 0;
}

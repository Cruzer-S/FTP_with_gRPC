#include <filesystem>
#include <iostream>
#include <variant>
#include <fstream>
#include <string>

#include <getopt.h>

#include <google/protobuf/util/time_util.h>
#include <google/protobuf/timestamp.pb.h>
#include <grpcpp/grpcpp.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

#include "protocol.grpc.pb.h"

#include "FileMetaData.hpp"
#include "FTPClient.hpp"

using ArgList = std::map<std::string, std::string>;

std::variant<ArgList, std::string> ParseArgument(int argc, char* argv[])
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
        if (argc < 4)
                return fmt::format("usage: {} <host> <service> <infile> <outpath>", *argv);

        argv += optind;

        arglist["host"] = *argv++;
        arglist["service"] = *argv++;
        arglist["infile"] = *argv++;
        arglist["outpath"] = *argv++;

        return arglist;
}

void ShowArgument(const ArgList& arglist)
{
        spdlog::info(fmt::format("Host: {}\tService: {}", arglist.at("host"), arglist.at("service")));
        spdlog::info(fmt::format("infile: {}", arglist.at("infile")));
        spdlog::info(fmt::format("outpath: {}", arglist.at("outpath")));
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

        const std::string target = fmt::format("{}:{}", arglist.at("host"), arglist.at("service"));
        std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
        spdlog::info("channel opened at: {}", target);

        FTPClient client(channel);

        const auto result = client.UploadFile(arglist.at("infile"), arglist.at("outpath"));
        if (const auto failed_to_upload = std::get_if<std::string>(&result)) {
                spdlog::error("failed to upload file: {}", *failed_to_upload);
                return 1;
        }

        spdlog::info("file uploaded successfully: \n{}", FileMetaDataToString(std::get<FileMetaData>(result)));

        return 0;
}
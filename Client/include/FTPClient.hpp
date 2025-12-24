#pragma once

#include <optional>
#include <fstream>
#include <variant>
#include <string>

#include <grpcpp/grpcpp.h>

#include "protocol.grpc.pb.h"

class FTPClient
{
public:
    FTPClient(std::shared_ptr<grpc::Channel> channel);

public:
    std::variant<FileMetaData, std::string> UploadFile(const std::string_view infile, const std::string_view outpath);

private:
    struct HashValue { std::string value; };
    enum ErrorType : bool {
        SERV_ERR = true,
        CLNT_ERR = false
    };

private:
    using WriterPtr = std::unique_ptr<grpc::ClientWriter<File>>;

    std::optional<std::pair<ErrorType, std::string>> SendFile(WriterPtr& writer, const std::string_view infile, const std::string_view outpath);

    std::optional<std::pair<ErrorType, std::string>> SendPath(WriterPtr& writer, const std::string_view outpath);
    std::variant<HashValue, std::pair<ErrorType, std::string>> SendChunk(WriterPtr& writer, const std::string_view infile);
    std::optional<std::pair<ErrorType, std::string>> SendHash(WriterPtr& writer, const HashValue& hash);

private:
    std::unique_ptr<FTPService::Stub> stub_;
};
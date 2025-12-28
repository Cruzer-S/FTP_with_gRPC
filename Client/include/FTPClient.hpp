#pragma once

#include "ftp_service.grpc.pb.h"

#include <optional>
#include <string>
#include <tuple>

#include <grpcpp/grpcpp.h>

#include "file.pb.h"
#include "hash.pb.h"
#include "ftp_service.pb.h"

class FTPClient
{
public:
	struct Error {
		int code;
		std::string message;
	};

public:
    FTPClient(std::shared_ptr<grpc::Channel> channel);

public:
    std::tuple<bool, FileMetaData, Error> UploadFile(const std::string &infile, const std::string &outpath);

private:
    using WriterPtr = std::unique_ptr<grpc::ClientWriter<UploadFileRequest>>;

    std::optional<Error> SendFile(WriterPtr& writer, const std::string_view infile, const std::string_view outpath);
    std::optional<Error> SendPath(WriterPtr& writer, const std::string_view infile, const std::string_view outpath);
    std::tuple<Hash, Error> SendChunk(WriterPtr& writer, const std::string_view infile);
    std::optional<Error> SendHash(WriterPtr& writer, const Hash& hash);

private:
    std::unique_ptr<FTPService::Stub> stub_;
};

#pragma once

#include "protocol.grpc.pb.h"

#include <optional>
#include <variant>
#include <atomic>
#include <string>
#include <map>

#include "google/protobuf/empty.pb.h"
#include "google/protobuf/timestamp.pb.h"

#include "HashFileStream.hpp"

class FTPServiceImpl final : public FTPService::Service
{
public:
        FTPServiceImpl(const std::string_view root_dir);

public:
        bool IsValid() const noexcept;

private:
        grpc::Status UploadFile(grpc::ServerContext* context, grpc::ServerReader<File>* reader, FileMetaData* response) override;

private:
        std::variant<grpc::Status, std::unique_ptr<HashFileStream>> OpenFile(grpc::ServerReader<File>* reader) noexcept;
        std::variant<grpc::Status, std::string> WriteToFile(grpc::ServerReader<File>* reader, std::unique_ptr<HashFileStream>& stream) noexcept;
        std::variant<grpc::Status, FileMetaData> CheckHash(std::unique_ptr<HashFileStream>& stream, const std::string_view hash) noexcept;

private:
        const std::string root_dir_;
};
#include "FTPServiceImpl.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <tuple>

#include <cstring>

#include "spdlog/fmt/bin_to_hex.h"
#include "spdlog/spdlog.h"
#include "grpcpp/grpcpp.h"

#include "HashFileStream.hpp"
#include "FileMetaData.hpp"

namespace fs = std::filesystem;

using StreamPtr = std::unique_ptr<HashFileStream>;

FTPServiceImpl::FTPServiceImpl(const std::string_view root_dir)
    : root_dir_(root_dir)
{
}

bool FTPServiceImpl::IsValid() const noexcept
{
    return fs::is_directory(root_dir_) && fs::exists(root_dir_);
}

grpc::Status FTPServiceImpl::UploadFile(grpc::ServerContext* context, grpc::ServerReader<File>* reader, FileMetaData* response)
{
    using Status = grpc::Status;

    spdlog::info("UploadFile() invoked");

    auto stream = OpenFile(reader);
    if (const auto status = std::get_if<Status>(&stream)) {
        spdlog::warn("failed to OpenFile(): {}", status->error_message());
        return *status;
    }
    spdlog::info("open file ({}) successfully", std::get<StreamPtr>(stream)->GetPath().c_str());

    auto hash = WriteToFile(reader, std::get<StreamPtr>(stream));
    if (const auto status = std::get_if<Status>(&hash)) {
        spdlog::warn("failed to WriteToFile(): {}", status->error_message());
        return *status;
    }
    spdlog::info("write all file chunk(s): hash {}", spdlog::to_hex(std::get<std::string>(hash).begin(), std::get<std::string>(hash).end()));

    if (const auto failed = std::get<StreamPtr>(stream)->Close()) {
        spdlog::warn("failed to close file: {}", *failed);
        return grpc::Status(grpc::StatusCode::INTERNAL, "failed to close file: {}", *failed);
    }

    auto metadata = CheckHash(std::get<StreamPtr>(stream), std::get<std::string>(hash));
    if (const auto status = std::get_if<Status>(&metadata)) {
        spdlog::warn("failed to CheckHash(): {}", status->error_message());
        return *status;
    }
    spdlog::info("hash check complete");

    response->CopyFrom(std::get<FileMetaData>(metadata));
    spdlog::info("upload file successfully: \n{}", FileMetaDataToString(std::get<FileMetaData>(metadata)));

    return Status::OK;
}

std::variant<grpc::Status, StreamPtr> FTPServiceImpl::OpenFile(grpc::ServerReader<File>* reader) noexcept
{
    File file; reader->Read(&file);

    if (!file.has_path())
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "You did not send path");
    if (file.has_hash() || file.has_chunk())
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "You send unncessary data");

    auto stream = std::make_unique<HashFileStream>(file.path(), Hasher(Hasher::Type::SHA256));
    if (const auto failed_to_open = stream->Open(std::ios::out | std::ios::binary | std::ios::trunc))
        return grpc::Status(grpc::StatusCode::INTERNAL, fmt::format("failed to open file({}): {}", file.path(), *failed_to_open));

    return std::move(stream);
}

std::variant<grpc::Status, std::string> FTPServiceImpl::WriteToFile(
    grpc::ServerReader<File>* reader, StreamPtr& stream
) noexcept
{
    using namespace grpc;

    File file;

    while (reader->Read(&file)) {
        if (file.has_path())
            return Status(StatusCode::INVALID_ARGUMENT, "You send unncessary data");

        if (!file.has_chunk()) {
            if (file.has_hash())
                return file.hash();

            return Status(StatusCode::INVALID_ARGUMENT, "You did not send chunk");
        }

        if (const auto failed_to_write = stream->Write(file.chunk()))
            return Status(StatusCode::INTERNAL, *failed_to_write);
    }

    return { Status(StatusCode::CANCELLED, "cancelled from client") };
}

std::variant<grpc::Status, FileMetaData> FTPServiceImpl::CheckHash(
    StreamPtr& stream, const std::string_view client_hash
) noexcept
{
    auto local_hash = stream->GetHash();
    if (!local_hash)
        return grpc::Status(grpc::StatusCode::DATA_LOSS, "failed to retrieve hash");

    if (local_hash->size() != client_hash.size())
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "incompatible hash type/size");

    if (std::memcmp(local_hash->data(), client_hash.data(), local_hash->size()))
        return grpc::Status(grpc::StatusCode::DATA_LOSS, "hash value does not match");

    return MakeFileMetaDataFrom(stream->GetPath(), *local_hash);
}
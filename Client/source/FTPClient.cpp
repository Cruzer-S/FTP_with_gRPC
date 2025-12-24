#include "FTPClient.hpp"

#include "fmt/core.h"

#include "HashFileStream.hpp"

FTPClient::FTPClient(std::shared_ptr<grpc::Channel> channel)
    : stub_(FTPService::NewStub(channel))
{

}

std::variant<FileMetaData, std::string> FTPClient::UploadFile(
    const std::string_view infile, const std::string_view outpath
) {
    grpc::ClientContext context;
    FileMetaData metadata;

    WriterPtr writer(stub_->UploadFile(&context, &metadata));

    if (const auto failed = SendFile(writer, infile, outpath)) {
        if (failed->first == CLNT_ERR)
            context.TryCancel();

        const grpc::Status status = writer->Finish();
        return fmt::format("{}: {} ({}-side)",
            status.error_message(), failed->second, failed->first == SERV_ERR ? "server" : "client"
        );
    }

    (void)writer->WritesDone();
    const grpc::Status status = writer->Finish();
    if (!status.ok())
        return fmt::format("{} (server-side)", status.error_message());

    return metadata;
}

std::optional<std::pair<FTPClient::ErrorType, std::string>> FTPClient::SendFile(
    WriterPtr& writer, const std::string_view infile, const std::string_view outpath
) {
    if (const auto failed = SendPath(writer, outpath))
        return std::pair{ failed->first, fmt::format("failed to SendPath(): {}", failed->second) };

    const auto result = SendChunk(writer, infile);
    if (const auto failed = std::get_if<std::pair<FTPClient::ErrorType, std::string>>(&result))
        return std::pair{ failed->first, fmt::format("failed to SendChunk(): {}", failed->second) };

    if (const auto failed = SendHash(writer, std::get<HashValue>(result)))
        return std::pair{ failed->first, fmt::format("failed to SendHash(): {}", failed->second) };

    return std::nullopt;
}

std::optional<std::pair<FTPClient::ErrorType, std::string>> FTPClient::SendPath(
    WriterPtr& writer, const std::string_view outpath
) {
    File file; file.Clear();

    *file.mutable_path() = outpath;

    if (!writer->Write(file))
        return std::pair{ SERV_ERR, "failed to send path" };

    return std::nullopt;
}

std::variant<FTPClient::HashValue, std::pair<FTPClient::ErrorType, std::string>> FTPClient::SendChunk(
    WriterPtr& writer, const std::string_view infile
) {
    File file; file.Clear();

    HashFileStream stream(infile, Hasher::SHA256);
    if (const auto failed = stream.Open(std::ios::binary | std::ios::in))
        return std::pair{ CLNT_ERR, fmt::format("failed to open file({}): {}", infile, *failed) };

    file.mutable_chunk()->resize(BUFSIZ);
    while (true) {
        const auto result = stream.Read(*file.mutable_chunk());
        if (const auto failed = std::get_if<std::string>(&result))
            return std::pair{ CLNT_ERR, fmt::format("failed to read file: {}", *failed) };

        if (!writer->Write(file))
            return std::pair{ SERV_ERR, fmt::format("failed to send chunk") };

        if (std::get<std::streamsize>(result) < file.mutable_chunk()->size())
            break;
    }

    if (const auto failed = stream.Close())
        return std::pair{ CLNT_ERR, fmt::format("failed to close file: {}", *failed) };

    const auto hash = stream.GetHash();
    if (!hash)
        return std::pair{ CLNT_ERR, fmt::format("failed to retrieve hash") };

    return HashValue{ *hash };
}

std::optional<std::pair<FTPClient::ErrorType, std::string>> FTPClient::SendHash(
    WriterPtr& writer, const HashValue& hash
) {
    File file; file.Clear();

    file.set_hash(hash.value);

    if (!writer->Write(file))
        return std::pair{ SERV_ERR, fmt::format("failed to send hash") };

    return std::nullopt;
}
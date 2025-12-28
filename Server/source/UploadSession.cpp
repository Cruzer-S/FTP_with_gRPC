#include "UploadSession.hpp"

std::optional<FileStream::Error> UploadSession::UploadSession::Open(std::ios::openmode mode) noexcept
{
    if (hashing) return hashing->Open(mode);
    if (plain)   return plain->Open(mode);
    return FileStream::Error{ -1, "session: no stream object" };
}

std::optional<FileStream::Error> UploadSession::UploadSession::Write(std::string_view data) noexcept
{
    if (hashing) return hashing->Write(data);
    if (plain)   return plain->Write(data);
    return FileStream::Error{ -1, "session: no stream object" };
}

std::optional<FileStream::Error> UploadSession::UploadSession::Close() noexcept
{
    if (hashing) return hashing->Close();
    if (plain)   return plain->Close();
    return std::nullopt;
}

std::optional<std::vector<uint8_t>> UploadSession::UploadSession::GetHash() const noexcept
{
    if (hashing) return hashing->GetHash();
    return std::nullopt;
}


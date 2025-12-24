#include "HashFileStream.hpp"

#include "fmt/core.h"

HashFileStream::HashFileStream(const std::filesystem::path& path, const Hasher& hasher)
    : FileStream(path), hasher_(hasher)
{
    hash_.clear();
}

HashFileStream::~HashFileStream()
{
}

std::optional<std::string> HashFileStream::Open(const std::ios::openmode& mode)
{
    if (const auto failed_to_init = hasher_.Init())
        return fmt::format("failed to initialize hasher: {}", *failed_to_init);

    if (const auto failed_to_open = FileStream::Open(mode))
        return failed_to_open;

    hash_.clear();

    return std::nullopt;
}

std::optional<std::string> HashFileStream::Write(const std::string_view data)
{
    if (const auto failed_to_hash = hasher_.Hash(data))
        return fmt::format("failed to hash: {}", *failed_to_hash);

    if (const auto failed_to_write = FileStream::Write(data))
        return failed_to_write;

    return std::nullopt;
}

std::variant<std::streamsize, std::string> HashFileStream::Read(std::string& data)
{
    return Read(data.data(), data.size());
}

std::variant<std::streamsize, std::string> HashFileStream::Read(char* data, const std::streamsize size)
{
    const auto result = FileStream::Read(data, size);
    if (const auto failed_to_read = std::get_if<std::string>(&result))
        return *failed_to_read;

    if (const auto failed_to_hash = hasher_.Hash(data, size))
        return *failed_to_hash;

    return std::get<std::streamsize>(result);
}

std::optional<std::string> HashFileStream::Close()
{
    const auto failed_to_finalize = hasher_.Final(hash_);
    if (failed_to_finalize)
        return failed_to_finalize;

    const auto failed_to_close = FileStream::Close();
    if (failed_to_close)
        return failed_to_close;

    return std::nullopt;
}

std::optional<std::string> HashFileStream::GetHash() const
{
    if (hash_.empty())
        return std::nullopt;

    return hash_;
}
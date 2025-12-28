#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "FileStream.hpp"
#include "Hasher.hpp"

class HashingFileStream final
{
public:
    using Error = FileStream::Error;

public:
    explicit HashingFileStream(const std::filesystem::path& path, Hasher::Type type);

public:
    const std::filesystem::path& GetPath() const noexcept;

public:
    std::optional<Error> Open(std::ios::openmode mode) noexcept;
    std::optional<Error> Write(std::string_view data) noexcept;

    std::tuple<bool, std::streamsize, Error> Read(std::string& data) noexcept;
    std::tuple<bool, std::streamsize, Error> Read(char* data, std::streamsize size) noexcept;

    std::optional<Error> Close() noexcept;

public:
    std::optional<std::vector<uint8_t>> GetHash() const noexcept;
    std::optional<std::string> GetHashHex() const;

private:
    static Error ConvertHasherError(const Hasher::Error& e);

private:
    FileStream file_;
    Hasher hasher_;
    std::optional<std::vector<uint8_t>> digest_;
};

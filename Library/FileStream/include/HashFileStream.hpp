#pragma once

#include "FileStream.hpp"

#include <filesystem>
#include <optional>
#include <fstream>
#include <variant>
#include <string>

#include "Hasher.hpp"

class HashFileStream final : public FileStream
{
public:
    HashFileStream(const std::filesystem::path& path, const Hasher& hasher);
    virtual ~HashFileStream();

public:
    std::optional<std::string> Open(const std::ios::openmode& mode) override;
    std::optional<std::string> Write(const std::string_view data) override;
    std::variant<std::streamsize, std::string> Read(std::string& data) override;
    std::variant<std::streamsize, std::string> Read(char* data, const std::streamsize size) override;
    std::optional<std::string> Close() override;

    std::optional<std::string> GetHash() const;

private:
    Hasher hasher_;
    std::string hash_;
};
#pragma once

#include <filesystem>
#include <optional>
#include <fstream>
#include <variant>
#include <string>

class FileStream
{
public:
    FileStream(const std::filesystem::path& path);
    ~FileStream();

public:
    const std::filesystem::path GetPath() const noexcept;

public:
    virtual std::optional<std::string> Open(const std::ios::openmode& mode);
    virtual std::optional<std::string> Write(const std::string_view data);
    virtual std::variant<std::streamsize, std::string> Read(std::string& data);
    virtual std::variant<std::streamsize, std::string> Read(char* data, const std::streamsize size);
    virtual std::optional<std::string> Close();

protected:
    std::string stream_error(const std::ios& stream) const;

private:
    const std::filesystem::path path_;
    std::fstream stream_;
};
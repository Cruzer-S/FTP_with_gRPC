#include "FileStream.hpp"

#include <cerrno>
#include <cstring>
#include <sstream>

FileStream::FileStream(const std::filesystem::path& path)
    : path_(path)
{
}

const std::filesystem::path& FileStream::GetPath() const noexcept
{
    return path_;
}

std::optional<FileStream::Error> FileStream::Open(std::ios::openmode mode) noexcept
{
    if (stream_.is_open())
        stream_.close();

    stream_.clear();

    errno = 0;
    stream_.open(path_, mode);
    if (!stream_.is_open() || stream_.fail() || stream_.bad())
        return stream_error(stream_, "open");

    return std::nullopt;
}

std::optional<FileStream::Error> FileStream::Write(std::string_view data) noexcept
{
    if (!stream_.is_open())
        return Error{-1, "write: stream is not open"};

    if (data.empty())
        return std::nullopt;

    stream_.write(data.data(), static_cast<std::streamsize>(data.size()));

    if (stream_.bad() || stream_.fail())
        return stream_error(stream_, "write");

    return std::nullopt;
}

std::tuple<bool, std::streamsize, FileStream::Error> FileStream::Read(std::string& data) noexcept
{
    if (data.empty())
        return { true, 0, Error{} };

    return Read(data.data(), static_cast<std::streamsize>(data.size()));
}

std::tuple<bool, std::streamsize, FileStream::Error> FileStream::Read(char* data, std::streamsize size) noexcept
{
    if (!stream_.is_open())
        return { false, 0, Error{ -1, "read: stream is not open"} };

    if (size < 0)
        return { false, 0, Error{ -1, "read: invalid size"} };

    if (size == 0)
        return { true, 0, Error{} };

    if (!data)
        return { false, 0, Error{ -1, "read: null buffer with non-zero size"} };

    stream_.read(data, size);
    const std::streamsize n = stream_.gcount();

    if (stream_.eof()) {
        stream_.clear(stream_.rdstate() & ~std::ios::eofbit);
        return { true, n, Error{} };
    }

    if (stream_.bad() || stream_.fail())
        return { false, n, stream_error(stream_, "read") };

    return { true, n, Error{} };
}

std::optional<FileStream::Error> FileStream::Close() noexcept
{
    if (!stream_.is_open())
        return std::nullopt;

    stream_.close();

    if (stream_.fail() || stream_.bad())
        return stream_error(stream_, "close");

    return std::nullopt;
}

FileStream::Error FileStream::stream_error(const std::ios& stream, const char* context) const noexcept
{
    const auto state = stream.rdstate();

    std::ostringstream oss;
    oss << (context ? context : "stream")
        << ": iostate=0x" << std::hex << static_cast<unsigned int>(state);

    if ((state & std::ios::badbit) != 0)  oss << " (badbit)";
    if ((state & std::ios::failbit) != 0) oss << " (failbit)";
    if ((state & std::ios::eofbit) != 0)  oss << " (eofbit)";

    if (errno != 0)
        oss << ", errno=" << errno << " (" << std::strerror(errno) << ")";

    oss << ", path=" << path_.string();

    return Error{ static_cast<int>(state), oss.str() };
}

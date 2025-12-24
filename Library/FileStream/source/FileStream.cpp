#include "FileStream.hpp"

#include <iostream>

#include <cstring>

FileStream::FileStream(const std::filesystem::path& path)
    : path_(path)
{

}

FileStream::~FileStream()
{
}

const std::filesystem::path FileStream::GetPath() const noexcept
{
    return path_;
}

std::optional<std::string> FileStream::Open(const std::ios::openmode& mode)
{
    stream_.open(path_, mode);
    if (!stream_.is_open())
        return stream_error(stream_);

    return std::nullopt;
}

std::optional<std::string> FileStream::Write(const std::string_view data)
{
    stream_.write(data.data(), data.size());
    stream_.flush();
    if (!stream_)
        return stream_error(stream_);

    return std::nullopt;
}

std::variant<std::streamsize, std::string> FileStream::Read(std::string& data)
{
    return Read(data.data(), data.capacity());
}

std::variant<std::streamsize, std::string> FileStream::Read(char* data, const std::streamsize size)
{
    stream_.read(data, size);
    const std::streamsize rlen = stream_.gcount();

    if (rlen > 0) {
        if (stream_.bad())
            return stream_error(stream_);

        return rlen;
    }

    if (stream_.eof())
        return std::streamsize(0);

    return stream_error(stream_);
}

std::optional<std::string> FileStream::Close()
{
    stream_.clear();

    stream_.close();
    if (stream_.fail())
        return stream_error(stream_);

    return std::nullopt;
}

std::string FileStream::stream_error(const std::ios& stream) const
{
    std::ostringstream oss;

    const int err = errno; // capture first

    const auto state = stream.rdstate();
    if (state == std::ios::goodbit)
        return std::string("stream state is good (no error flags set)");

    oss << "stream error:";
    if (state & std::ios::failbit) oss << " failbit";
    if (state & std::ios::badbit)  oss << " badbit";
    if (state & std::ios::eofbit)  oss << " eofbit";

    if (err != 0)
        oss << " (errno=" << err << ": " << std::strerror(err) << ")";
    else
        oss << " (errno not set/unknown)";

    errno = 0;

    return oss.str();
}
#include "HashingFileStream.hpp"

#include <sstream>

HashingFileStream::HashingFileStream(const std::filesystem::path& path, Hasher::Type type)
    : file_(path)
    , hasher_(type)
{
}

const std::filesystem::path& HashingFileStream::GetPath() const noexcept
{
    return file_.GetPath();
}

std::optional<HashingFileStream::Error> HashingFileStream::Open(std::ios::openmode mode) noexcept
{
    digest_.reset();

    if (auto err = file_.Open(mode))
        return err;

    if (auto herr = hasher_.Initialize()) {
        (void)file_.Close();
        return ConvertHasherError(*herr);
    }

    return std::nullopt;
}

std::optional<HashingFileStream::Error> HashingFileStream::Write(std::string_view data) noexcept
{
    if (auto err = file_.Write(data))
        return err;

    if (data.empty())
        return std::nullopt;

    if (auto herr = hasher_.Update(data.data(), data.size()))
        return ConvertHasherError(*herr);

    return std::nullopt;
}

std::tuple<bool, std::streamsize, HashingFileStream::Error>
HashingFileStream::Read(std::string& data) noexcept
{
    auto [ok, n, err] = file_.Read(data);
    if (!ok)
        return { false, n, err };

    if (n >= 0 && static_cast<size_t>(n) <= data.size())
        data.resize(static_cast<size_t>(n));

    if (n <= 0)
        return { true, n, Error{} };

    if (auto herr = hasher_.Update(data.data(), static_cast<size_t>(n)))
        return { false, n, ConvertHasherError(*herr) };

    return { true, n, Error{} };
}

std::tuple<bool, std::streamsize, HashingFileStream::Error>
HashingFileStream::Read(char* data, std::streamsize size) noexcept
{
    auto [ok, n, err] = file_.Read(data, size);
    if (!ok)
        return { false, n, err };

    if (n <= 0)
        return { true, n, Error{} };

    if (auto herr = hasher_.Update(data, static_cast<size_t>(n)))
        return { false, n, ConvertHasherError(*herr) };

    return { true, n, Error{} };
}

std::optional<HashingFileStream::Error> HashingFileStream::Close() noexcept
{
    auto [ok, digest, herr] = hasher_.Finalize();
    if (!ok) {
	(void)file_.Close();
	return ConvertHasherError(herr);
    }

    digest_ = std::move(digest);
    if (auto err = file_.Close())
        return err;

    return std::nullopt;
}

std::optional<std::vector<uint8_t>> HashingFileStream::GetHash() const noexcept
{
    return digest_;
}

std::optional<std::string> HashingFileStream::GetHashHex() const
{
    if (!digest_.has_value())
        return std::nullopt;

    static const char* kHex = "0123456789abcdef";
    const auto& d = *digest_;

    std::string out;
    out.reserve(d.size() * 2);
    for (uint8_t b : d) {
        out.push_back(kHex[(b >> 4) & 0xF]);
        out.push_back(kHex[b & 0xF]);
    }

    return out;
}

HashingFileStream::Error HashingFileStream::ConvertHasherError(const Hasher::Error& e)
{
    std::ostringstream oss;
    oss << "hasher: (" << e.code << ") " << e.message;
    return Error{ e.code, oss.str() };
}

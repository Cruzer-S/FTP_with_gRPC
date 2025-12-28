#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>

class FileStream {
public:
	struct Error {
		int code = 0;
		std::string message;
	};

public:
	explicit FileStream(const std::filesystem::path& path);
	virtual ~FileStream() = default;

public:
	const std::filesystem::path& GetPath() const noexcept;

public:
	virtual std::optional<Error> Open(std::ios::openmode mode) noexcept;
	virtual std::optional<Error> Write(std::string_view data) noexcept;

	virtual std::tuple<bool, std::streamsize, Error> Read(std::string& data) noexcept;
	virtual std::tuple<bool, std::streamsize, Error> Read(char* data, std::streamsize size) noexcept;

	virtual std::optional<Error> Close() noexcept;

private:
	Error stream_error(const std::ios& stream, const char* context) const noexcept;

private:
	const std::filesystem::path path_;
	std::fstream stream_;
};

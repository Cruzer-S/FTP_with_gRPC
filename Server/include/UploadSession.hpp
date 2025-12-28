#pragma once

#include <filesystem>
#include <optional>

#include "FileStream.hpp"
#include "HashingFileStream.hpp"

#include "hash.pb.h"

struct UploadSession {
	std::filesystem::path path;
	uint64_t expected_size;

	bool touch_only = false;
	bool hashing_enabled = false;

	std::unique_ptr<FileStream> plain;
	std::unique_ptr<HashingFileStream> hashing;

	HashType hash_type = HASH_TYPE_UNSPECIFIED;

	std::optional<FileStream::Error> Open(std::ios::openmode mode) noexcept;
	std::optional<FileStream::Error> Write(std::string_view data) noexcept;
	std::optional<FileStream::Error> Close() noexcept;
	std::optional<std::vector<uint8_t>> GetHash() const noexcept;
};

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "openssl/evp.h"

class Hasher
{
public:
	enum class Type {
		SHA256 = 0,
		SHA512
	};

	struct Error {
		int code;
		std::string message;
	};

public:
	// Can be moved, but can't be copied
	Hasher(const Hasher&) = delete;
	Hasher& operator=(const Hasher&) = delete;
	Hasher(Hasher&&) noexcept;
	Hasher& operator=(Hasher&&) noexcept;

public:
	Hasher(Type type);
	~Hasher();

public:
	std::optional<Error> Initialize() noexcept;
    	std::optional<Error> Update(const char* buffer, const size_t size) noexcept;
	std::optional<Error> Update(const std::string &data) noexcept;
	std::tuple<bool, std::vector<uint8_t>, Error> Finalize() noexcept;

private:
    Type type_;
    const EVP_MD* md_;
    EVP_MD_CTX *ctx_;
};

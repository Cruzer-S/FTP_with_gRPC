#include "Hasher.hpp"

#include "fmt/core.h"

#include "openssl/sha.h"
#include "openssl/err.h"

namespace {
	Hasher::Error GetLastError(const std::string &what) noexcept
	{
		char buffer[BUFSIZ];
		
		const unsigned long err = ERR_get_error();
		if (err == 0)
			snprintf(buffer, BUFSIZ, "OpenSSL error (no queued error)");
		else
			ERR_error_string_n(err, buffer, BUFSIZ);


		Hasher::Error error;
		error.code = static_cast<int>(err);
		error.message = what + ": " + std::string(buffer);

        	return error;
	}

	Hasher::Error MakeError(int code, const char *message) noexcept
	{
		Hasher::Error error;

		error.code = code;
		error.message = std::string(message);

		return error;
	}

	const EVP_MD* ResolveMD(Hasher::Type type)
	{
		switch (type) {
		case Hasher::Type::SHA256:
			return EVP_sha256();
		case Hasher::Type::SHA512:
			return EVP_sha512();
		}

		return nullptr;
	}

	size_t GetMDSize(Hasher::Type type)
	{
		switch (type) {
		case Hasher::Type::SHA256:
			return SHA256_DIGEST_LENGTH;
		case Hasher::Type::SHA512:
			return SHA512_DIGEST_LENGTH;
		}

		return 0;
	}
}

Hasher::Hasher(Type type)
	: type_(type)
	, md_(ResolveMD(type))
	, ctx_(nullptr)
{
}

Hasher::~Hasher()
{
	if (ctx_) {
		EVP_MD_CTX_free(ctx_);
		ctx_ = nullptr;
	}
}

Hasher::Hasher(Hasher&& other) noexcept
	: type_(other.type_)
	, md_(other.md_)
	, ctx_(other.ctx_)
{
	other.md_ = nullptr;
	other.ctx_ = nullptr;
}

Hasher& Hasher::operator=(Hasher&& other) noexcept
{
	if (this == &other)
		return *this;

	if (ctx_) {
		EVP_MD_CTX_free(ctx_);
		ctx_ = nullptr;
	}

	type_ = other.type_;
	md_   = other.md_;
	ctx_  = other.ctx_;

	other.md_  = nullptr;
	other.ctx_ = nullptr;

	return *this;
}

std::optional<Hasher::Error> Hasher::Initialize() noexcept
{
	if (!md_)
		return MakeError(-1, "Unsupported hash type");

	ctx_ = EVP_MD_CTX_new();
	if (!ctx_)
		return GetLastError("EVP_MD_CTX_new failed");

	if (EVP_DigestInit_ex(ctx_, md_, nullptr) != 1)
		return GetLastError("EVP_DigestInit_ex failed");

	return std::nullopt;
}

std::optional<Hasher::Error> Hasher::Update(const char* buffer,
					    const size_t size) noexcept
{
	if (!buffer && size != 0)
		return MakeError(-3, "Update received null buffer with non-zero size");

	if (size == 0)
		return std::nullopt;

	if (EVP_DigestUpdate(ctx_, buffer, size) != 1)
		return GetLastError("EVP_DigestUpdate failed");

	return std::nullopt;
}

std::optional<Hasher::Error> Hasher::Update(const std::string& data) noexcept
{
	return Update(data.data(), data.size());
}

std::tuple<bool, std::vector<uint8_t>, Hasher::Error> Hasher::Finalize() noexcept
{
	unsigned int out_len = EVP_MD_size(md_);
	if (out_len == 0U || out_len > 1024U) {
		return {
			false,
			{},
			MakeError(-4, "Invalid digest size")
		};
	}

	std::vector<uint8_t> digest(out_len);

	if (EVP_DigestFinal_ex(ctx_, digest.data(), &out_len) != 1) {
		return {
			false,
			{},
			GetLastError("EVP_DigestFinal_ex failed")
		};
	}

	digest.resize(out_len);

	return { true, std::move(digest), Hasher::Error{0, ""} };
}

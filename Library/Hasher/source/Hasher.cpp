#include "Hasher.hpp"

#include "fmt/core.h"

namespace {
    const std::string last_error_string()
    {
        char buffer[BUFSIZ];

        unsigned long err = ERR_get_error();
        if (err == 0)
            return std::string("OpenSSL error queue is empty");

        ERR_error_string_n(err, buffer, BUFSIZ);

        return std::string(buffer);
    }

    const EVP_MD* GetMD(Hasher::Type type)
    {
        switch (type) {
        case Hasher::Type::SHA256:
            return EVP_sha256();
        }

        return nullptr;
    }

    size_t GetMDSize(Hasher::Type type)
    {
        switch (type) {
        case Hasher::Type::SHA256:
            return SHA256_DIGEST_LENGTH;
        }

        return 0;
    }
}

Hasher::Hasher(Type type)
    : type_(type)
    , md_(GetMD(type_))
    , ctx_(nullptr)
{
}

Hasher::~Hasher()
{
    if (ctx_ != nullptr)
        EVP_MD_CTX_free(ctx_);
}

std::optional<std::string> Hasher::Init()
{
    if (!md_)
        return std::string("Unknown EVP_MD type");

    ctx_ = EVP_MD_CTX_new();
    if (!ctx_)
        return fmt::format("failed to EVP_MD_CTX_new(): ", last_error_string());

    if (EVP_DigestInit_ex(ctx_, md_, nullptr) != 1) {
        EVP_MD_CTX_free(ctx_);
        ctx_ = nullptr;
        return fmt::format("failed to EVP_DigestInit_ex(): ", last_error_string());
    }

    return std::nullopt;
}

std::optional<std::string> Hasher::Hash(const char* buffer, const size_t size)
{
    if (EVP_DigestUpdate(ctx_, buffer, size) != 1)
        return fmt::format("failed to EVP_DigestUpdate(): ", last_error_string());

    return std::nullopt;
}

std::optional<std::string> Hasher::Hash(const std::string_view data)
{
    return Hash(data.data(), data.size());
}

std::optional<std::string> Hasher::Final(std::string& out)
{
    unsigned int md_len = 0;

    out.resize(GetMDSize(type_));
    if (EVP_DigestFinal_ex(ctx_, (unsigned char*)out.data(), &md_len) != 1)
        return fmt::format("failed to EVP_DigestFinal_ex(): {}", last_error_string());

    return std::nullopt;
}
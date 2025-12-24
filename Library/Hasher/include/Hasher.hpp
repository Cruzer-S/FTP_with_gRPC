#pragma once

#include <optional>
#include <string>

#include "openssl/evp.h"
#include "openssl/sha.h"
#include "openssl/err.h"

class Hasher
{
public:
    enum Type {
        SHA256 = 0,
    };

public:
    Hasher(Type type);
    ~Hasher();

public:
    std::optional<std::string> Init();
    std::optional<std::string> Hash(const char* buffer, const size_t size);
    std::optional<std::string> Hash(const std::string_view data);
    std::optional<std::string> Final(std::string& out);

private:
    const Type type_;
    const EVP_MD* md_;
    EVP_MD_CTX* ctx_;
};
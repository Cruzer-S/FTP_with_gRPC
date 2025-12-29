#include "FTPServiceImpl.hpp"

#include <system_error>
#include <string_view>
#include <filesystem>
#include <optional>
#include <string>
#include <tuple>

#include <cstring>

#include "FileMetaData.hpp"

#include "ftp_service.pb.h"
#include "file.pb.h"

#include "grpcpp/support/status.h"

namespace fs = std::filesystem;

namespace {
	static std::optional<Hasher::Type> MapHasherType(HashType t) noexcept
	{
		switch (t) {
		case HASH_TYPE_SHA256: return Hasher::Type::SHA256;
		case HASH_TYPE_SHA512: return Hasher::Type::SHA512;
		default: return std::nullopt;
		}
	}

	static grpc::Status InvalidArg(std::string msg)
	{
		return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, std::move(msg));
	}

	static grpc::Status Internal(std::string msg)
	{
		return grpc::Status(grpc::StatusCode::INTERNAL, std::move(msg));
	}

	static std::optional<Hasher::Type> MapHashTypeOptional(const UploadInit& init)
	{
		if (!init.has_hashtype())
			return std::nullopt;

		switch (init.hashtype()) {
		case HASH_TYPE_SHA256: return Hasher::Type::SHA256;
		case HASH_TYPE_SHA512: return Hasher::Type::SHA512;
		case HASH_TYPE_UNSPECIFIED:
		default:
			return std::nullopt;
		}
	}

	static bool HashLengthMatches(HashType t, size_t n)
	{
		switch (t) {
		case HASH_TYPE_SHA256: return n == 32;
		case HASH_TYPE_SHA512: return n == 64;
		case HASH_TYPE_UNSPECIFIED:
		default:
			return n == 0;
		}
	}
}

FTPServiceImpl::FTPServiceImpl(const std::string_view root_dir)
    : root_dir_(root_dir)
{
}

bool FTPServiceImpl::IsValid() const noexcept
{
    std::error_code ec;
    return !root_dir_.empty()
        && fs::exists(root_dir_, ec)
        && fs::is_directory(root_dir_, ec);
}

grpc::Status FTPServiceImpl::UploadFile(grpc::ServerContext* context,
                                        grpc::ServerReader<UploadFileRequest>* reader,
                                        UploadFileResponse* response)
{
    auto [ok_open, session, st_open] = OpenFile(reader);
    if (!ok_open)
        return st_open;

    auto [ok_write, st_write] = WriteToFile(reader, session);
    if (!ok_write)
            return st_write;

    auto [ok_hash, metadata, st_meta] = CheckHash(reader, session);
    if (!ok_hash)
        return st_meta;

    *response->mutable_metadata() = std::move(metadata);
    Hash hash_out;
    hash_out.set_hashtype(session.hash_type);
    hash_out.set_data(session.GetHash()->data(), session.GetHash()->size());
    *response->mutable_hash() = hash_out;

    return grpc::Status::OK;
}

std::tuple<bool, UploadSession, grpc::Status>
FTPServiceImpl::OpenFile(grpc::ServerReader<UploadFileRequest>* reader) noexcept
{
    UploadSession session;

    UploadFileRequest first;
    if (!reader->Read(&first))
        return { false, std::move(session), InvalidArg("empty request stream") };

    if (first.request_case() != UploadFileRequest::kInit)
        return { false, std::move(session), InvalidArg("first message must be init") };

    const UploadInit& init = first.init();

    if (init.filepath().empty())
		return { false, std::move(session), InvalidArg("init.filepath is empty") };

    const std::filesystem::path path = init.filepath();
    if (!path.is_absolute())
		return { false, std::move(session), InvalidArg("init.filepath must be an absolute path") };

    if (!std::filesystem::exists(path.parent_path()))
		return { false, std::move(session), InvalidArg("init.filepath can't be created (no such directory)") };

    session.path = path;

    session.touch_only = !init.has_filesize();
    session.expected_size = init.has_filesize() ? init.filesize() : 0;

    session.hashing_enabled = init.has_hashtype() && (init.hashtype() != HASH_TYPE_UNSPECIFIED);
    session.hash_type = session.hashing_enabled ? init.hashtype() : HASH_TYPE_UNSPECIFIED;

    if (session.hashing_enabled && session.touch_only)
		return { false, std::move(session), InvalidArg("") };

    if (session.hashing_enabled) {
        auto opt = MapHasherType(session.hash_type);
        if (!opt)
            return { false, std::move(session), InvalidArg("invalid hashtype") };

        session.hashing = std::make_unique<HashingFileStream>(session.path, *opt);
    } else {
        session.plain = std::make_unique<FileStream>(session.path);
    }

    if (auto err = session.Open(std::ios::binary | std::ios::out | std::ios::trunc))
       return { false, std::move(session), Internal("open failed: " + err->message) };

    return { true, std::move(session), grpc::Status::OK };
}

std::tuple<bool, grpc::Status> FTPServiceImpl::WriteToFile(grpc::ServerReader<UploadFileRequest>* reader, UploadSession& session) noexcept
{
    const uint64_t expected = session.expected_size;
    uint64_t total = 0;

    UploadFileRequest req;
    while (total < expected && reader->Read(&req)) {
        switch (req.request_case()) {
        case UploadFileRequest::kChunk: {
            const std::string& data = req.chunk().data();
            if (data.empty())
				break;

            const uint64_t add = static_cast<uint64_t>(data.size());
            if (total + add > expected)
                return { false, InvalidArg("received more bytes than filesize") };

            if (auto err = session.Write(std::string_view(data.data(), data.size())))
                return { false, Internal("write failed: " + err->message) };

            total += add;
            break;
        }

        case UploadFileRequest::kFinish:
            return { false, InvalidArg("finish must appear only as the last message") };

        case UploadFileRequest::kInit:
            return { false, InvalidArg("init must appear only as the first message") };

        case UploadFileRequest::REQUEST_NOT_SET:
        default:
            return { false, InvalidArg("invalid request") };
        }

        if (total == expected)
            break;
    }

    if (total != expected)
        return { false, InvalidArg("stream ended before receiving filesize bytes") };

    if (auto err = session.Close())
        return { false, Internal("close failed: " + err->message) };

    return { true, grpc::Status::OK };
}

std::tuple<bool, FileMetaData, grpc::Status>
FTPServiceImpl::CheckHash(grpc::ServerReader<UploadFileRequest>* reader, const UploadSession& session) noexcept
{
    const FileMetaData metadata = MakeFileMetaDataFrom(session.path);

    if (!session.hashing_enabled) {
		if (session.touch_only)
			return { true, std::move(metadata), grpc::Status::OK };

		if (metadata.size() != session.expected_size)
			return { false, FileMetaData{}, InvalidArg("file size mismatch after write") };

		return { true, std::move(metadata), grpc::Status::OK };
    }

    UploadFileRequest last;
    if (!reader->Read(&last))
        return { false, FileMetaData{}, InvalidArg("failed to read last request") };

    UploadFileRequest extra;
    if (!reader->Read(&last))
        return { false, FileMetaData{}, InvalidArg("extra messages after finish are not allowed") };

    if (last.request_case() != UploadFileRequest::kFinish)
		return { false, FileMetaData{}, InvalidArg("finish must be the last message") };

    if (!last.finish().has_hash())
        return { false, FileMetaData{}, InvalidArg("failed to read hash") };

    const auto hash_opt = session.GetHash();
    if (!hash_opt)
		return { false, FileMetaData{}, Internal("failed to read server hash") };

    const std::vector<uint8_t> server_hash = *hash_opt;
    const Hash &expected = last.finish().hash();
    if (expected.hashtype() != session.hash_type)
        return { false, FileMetaData{}, InvalidArg("finish.hash.hashtype mismatch with init.hashtype") };

    if (!HashLengthMatches(expected.hashtype(), static_cast<size_t>(expected.data().size())))
        return { false, FileMetaData{}, InvalidArg("finish.hash.data length does not match hashtype") };

    if (expected.data().size() != server_hash.size()  ||
        std::memcmp(expected.data().data(), server_hash.data(), expected.data().size()) != 0)
		return { false, FileMetaData{}, grpc::Status(grpc::StatusCode::DATA_LOSS, "hash mismatch") };
    
    return { true, std::move(metadata), grpc::Status::OK };
}

#include "FTPClient.hpp"

#include <string_view>
#include <filesystem>
#include <vector>
#include <memory>
#include <string>
#include <tuple>

#include <cstdint>
#include <cstring>

#include <grpcpp/grpcpp.h>

#include "Hasher.hpp"
#include "ftp_service.pb.h"
#include "hash.pb.h"
#include "file.pb.h"

#include "HashingFileStream.hpp"

namespace {
	static FTPClient::Error OkError()
	{
		return FTPClient::Error{ 0, "" };
	}

	static FTPClient::Error MakeErr(int code, std::string msg)
	{
		return FTPClient::Error{ code, std::move(msg) };
	}

	static FTPClient::Error MakeGrpcErr(const grpc::Status& st)
	{
		return FTPClient::Error{ static_cast<int>(st.error_code()), st.error_message() };
	}

	static std::optional<Hasher::Type> MapHashTypeOptional(HashType hashtype)
	{
		switch (hashtype) {
		case HASH_TYPE_SHA256: return Hasher::Type::SHA256;
		case HASH_TYPE_SHA512: return Hasher::Type::SHA512;
		case HASH_TYPE_UNSPECIFIED:
		default:
			return std::nullopt;
		}

		return std::nullopt;
	}
}

FTPClient::FTPClient(std::shared_ptr<grpc::Channel> channel)
    : stub_(FTPService::NewStub(std::move(channel)))
{
}

std::tuple<bool, FileMetaData, FTPClient::Error>
FTPClient::UploadFile(const std::string& infile, const std::string& outpath, const HashType &hashtype)
{
    if (!stub_)
        return { false, FileMetaData{}, MakeErr(-1, "stub not initialized") };

    if (infile.empty() || outpath.empty())
        return { false, FileMetaData{}, MakeErr(-1, "infile/outpath is empty") };

    grpc::ClientContext ctx;
    UploadFileResponse resp;

    WriterPtr writer = stub_->UploadFile(&ctx, &resp);
    if (!writer)
        return { false, FileMetaData{}, MakeErr(-1, "failed to create ClientWriter") };

    if (auto err = SendPath(writer, infile, outpath)) {
        ctx.TryCancel();
        return { false, FileMetaData{}, *err };
    }

    auto [hash, herr] = SendChunk(writer, infile, hashtype);
    if (herr.code != 0) {
        ctx.TryCancel();
        return { false, FileMetaData{}, herr };
    }

    if (auto err = SendHash(writer, hash)) {
        ctx.TryCancel();
        return { false, FileMetaData{}, *err };
    }

    writer->WritesDone();
    grpc::Status st = writer->Finish();
    if (!st.ok())
        return { false, FileMetaData{}, MakeGrpcErr(st) };

    if (resp.hash().hashtype() != HASH_TYPE_UNSPECIFIED && !resp.hash().data().empty())
        if (resp.hash().hashtype() != hash.hashtype() || resp.hash().data() != hash.data())
            return { false, FileMetaData{}, MakeErr(-1, "server returned hash mismatch") };

    return { true, resp.metadata(), OkError() };
}

std::optional<FTPClient::Error>
FTPClient::SendFile(WriterPtr& writer, const std::string_view infile, const std::string_view outpath, const HashType &hashtype)
{
    if (auto err = SendPath(writer, infile, outpath))
        return err;

    auto [hash, herr] = SendChunk(writer, infile, hashtype);
    if (herr.code != 0)
        return herr;

    if (auto err = SendHash(writer, hash))
        return err;

    return std::nullopt;
}

std::optional<FTPClient::Error>
FTPClient::SendPath(WriterPtr& writer,
                    const std::string_view infile,
                    const std::string_view outpath)
{
    uintmax_t size = std::filesystem::file_size(infile);
    UploadFileRequest req;
    UploadInit init;

    init.set_filepath(std::string(outpath));
    init.set_filesize(size);

    init.set_hashtype(HASH_TYPE_SHA256);

    *req.mutable_init() = std::move(init);

    if (!writer->Write(req))
        return MakeErr(-1, "failed to write init");

    return std::nullopt;
}

std::tuple<Hash, FTPClient::Error> FTPClient::SendChunk(
	WriterPtr& writer, const std::string_view infile,
	const HashType &hashtype
) {
    HashingFileStream stream(infile, *MapHashTypeOptional(hashtype));
    if (const auto &error = stream.Open(std::ios::binary | std::ios::in))
        return { Hash{}, MakeErr(-1, "failed to open infile: " + error->message) };
    
    constexpr std::size_t kChunkSize = 64 * BUFSIZ;
    std::vector<char> buffer(kChunkSize);

    std::uint64_t offset = 0;

    while (true) {
        const auto &[ok, len, err] = stream.Read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
		if (!ok)
			return { Hash{}, MakeErr(-1, "failed to read infile: " + err.message) };

		if (len <= 0)
			break;
        
        UploadFileRequest req;
        UploadChunk chunk;

        chunk.set_data(buffer.data(), static_cast<std::size_t>(len));
        chunk.set_offset(offset);

        *req.mutable_chunk() = std::move(chunk);

        if (!writer->Write(req))
            return { Hash{}, MakeErr(-1, "failed to write chunk") };

        offset += static_cast<std::uint64_t>(len);
    }

	if (const auto err = stream.Close())
		return { Hash{}, MakeErr(err->code, "failed to close infile: " + err->message) };

    Hash hash;
    hash.set_hashtype(HASH_TYPE_SHA256);
    hash.set_data(stream.GetHash()->data(), stream.GetHash()->size());

    return { std::move(hash), OkError() };
}

std::optional<FTPClient::Error>
FTPClient::SendHash(WriterPtr& writer, const Hash& hash)
{
    UploadFileRequest req;
    UploadFinish fin;

    *fin.mutable_hash() = hash;
    *req.mutable_finish() = std::move(fin);

    if (!writer->Write(req))
        return MakeErr(-1, "failed to write finish");

    return std::nullopt;
}

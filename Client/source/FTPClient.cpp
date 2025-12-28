#include "FTPClient.hpp"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "Hasher.hpp"
#include "ftp_service.pb.h"
#include "hash.pb.h"
#include "file.pb.h"

namespace {

static FTPClient::Error OkError()
{
    return FTPClient::Error{0, ""};
}

static FTPClient::Error MakeErr(int code, std::string msg)
{
    return FTPClient::Error{code, std::move(msg)};
}

static FTPClient::Error MakeGrpcErr(const grpc::Status& st)
{
    return FTPClient::Error{static_cast<int>(st.error_code()), st.error_message()};
}

static std::tuple<bool, std::uint64_t, FTPClient::Error> GetFileSize(const std::string& infile)
{
    std::ifstream in(infile, std::ios::binary);
    if (!in.is_open()) {
        return {false, 0, MakeErr(-1, "failed to open infile for size: " + infile)};
    }

    in.seekg(0, std::ios::end);
    const auto end = in.tellg();
    if (end < 0) {
        return {false, 0, MakeErr(-1, "failed to tellg infile: " + infile)};
    }

    return {true, static_cast<std::uint64_t>(end), OkError()};
}

} // namespace

FTPClient::FTPClient(std::shared_ptr<grpc::Channel> channel)
    : stub_(FTPService::NewStub(std::move(channel)))
{
}

std::tuple<bool, FileMetaData, FTPClient::Error>
FTPClient::UploadFile(const std::string& infile, const std::string& outpath)
{
    if (!stub_) {
        return {false, FileMetaData{}, MakeErr(-1, "stub not initialized")};
    }
    if (infile.empty() || outpath.empty()) {
        return {false, FileMetaData{}, MakeErr(-1, "infile/outpath is empty")};
    }

    grpc::ClientContext ctx;
    UploadFileResponse resp;

    WriterPtr writer = stub_->UploadFile(&ctx, &resp);
    if (!writer) {
        return {false, FileMetaData{}, MakeErr(-1, "failed to create ClientWriter")};
    }

    // 1) init (filepath + filesize + hashtype)
    if (auto err = SendPath(writer, infile, outpath)) {
        ctx.TryCancel();
        return {false, FileMetaData{}, *err};
    }

    // 2) chunks + local hash
    auto [hash, herr] = SendChunk(writer, infile);
    if (herr.code != 0) {
        ctx.TryCancel();
        return {false, FileMetaData{}, herr};
    }

    // 3) finish (expected hash for server-side verification)
    if (auto err = SendHash(writer, hash)) {
        ctx.TryCancel();
        return {false, FileMetaData{}, *err};
    }

    writer->WritesDone();
    grpc::Status st = writer->Finish();
    if (!st.ok()) {
        return {false, FileMetaData{}, MakeGrpcErr(st)};
    }

    // optional: compare response.hash with our hash if server filled it
    // (proto3에서는 has_xxx가 없을 수 있으므로 내용 기반 방어)
    if (resp.hash().hashtype() != HASH_TYPE_UNSPECIFIED && !resp.hash().data().empty()) {
        if (resp.hash().hashtype() != hash.hashtype() || resp.hash().data() != hash.data()) {
            return {false, FileMetaData{}, MakeErr(-1, "server returned hash mismatch")};
        }
    }

    return {true, resp.metadata(), OkError()};
}

std::optional<FTPClient::Error>
FTPClient::SendFile(WriterPtr& writer, const std::string_view infile, const std::string_view outpath)
{
    // 이 함수는 UploadFile()이 이미 orchestration을 하므로,
    // 필요하면 UploadFile() 내부 호출로 유지할 수 있습니다.
    if (auto err = SendPath(writer, infile, outpath)) {
        return err;
    }

    auto [hash, herr] = SendChunk(writer, infile);
    if (herr.code != 0) {
        return herr;
    }

    if (auto err = SendHash(writer, hash)) {
        return err;
    }

    return std::nullopt;
}

std::optional<FTPClient::Error>
FTPClient::SendPath(WriterPtr& writer,
                    const std::string_view infile,
                    const std::string_view outpath)
{
    auto [ok, size, e] = GetFileSize(std::string(infile));
    if (!ok) {
        return e;
    }

    UploadFileRequest req;
    UploadInit init;

    init.set_filepath(std::string(outpath));
    init.set_filesize(size);

    // 해시 검증을 원하므로 hashtype도 같이 보냄 (SHA256 고정)
    init.set_hashtype(HASH_TYPE_SHA256);

    *req.mutable_init() = std::move(init);

    if (!writer->Write(req)) {
        return MakeErr(-1, "failed to write init");
    }

    return std::nullopt;
}

std::tuple<Hash, FTPClient::Error>
FTPClient::SendChunk(WriterPtr& writer, const std::string_view infile)
{
    std::ifstream in(std::string(infile), std::ios::binary);
    if (!in.is_open()) {
        return {Hash{}, MakeErr(-1, "failed to open infile: " + std::string(infile))};
    }

    Hasher hasher(Hasher::Type::SHA256);
    if (auto e = hasher.Initialize()) {
        return {Hash{}, MakeErr(e->code, "hasher initialize failed: " + e->message)};
    }

    constexpr std::size_t kChunkSize = 256 * 1024;
    std::vector<char> buf(kChunkSize);

    std::uint64_t offset = 0;

    while (in) {
        in.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        const std::streamsize n = in.gcount();
        if (n <= 0) break;

        if (auto e = hasher.Update(buf.data(), static_cast<std::size_t>(n))) {
            return {Hash{}, MakeErr(e->code, "hasher update failed: " + e->message)};
        }

        UploadFileRequest req;
        UploadChunk chunk;

        chunk.set_data(buf.data(), static_cast<std::size_t>(n));
        chunk.set_offset(offset);

        *req.mutable_chunk() = std::move(chunk);

        if (!writer->Write(req)) {
            return {Hash{}, MakeErr(-1, "failed to write chunk")};
        }

        offset += static_cast<std::uint64_t>(n);
    }

    auto [ok, digest, err] = hasher.Finalize();
    if (!ok) {
        return {Hash{}, MakeErr(err.code, "hasher finalize failed: " + err.message)};
    }

    Hash h;
    h.set_hashtype(HASH_TYPE_SHA256);
    h.set_data(digest.data(), digest.size());
    return {std::move(h), OkError()};
}

std::optional<FTPClient::Error>
FTPClient::SendHash(WriterPtr& writer, const Hash& hash)
{
    UploadFileRequest req;
    UploadFinish fin;

    *fin.mutable_hash() = hash;
    *req.mutable_finish() = std::move(fin);

    if (!writer->Write(req)) {
        return MakeErr(-1, "failed to write finish");
    }
    return std::nullopt;
}

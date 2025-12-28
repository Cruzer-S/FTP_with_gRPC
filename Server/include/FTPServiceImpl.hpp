#pragma once

#include "ftp_service.grpc.pb.h"
#include "ftp_service.pb.h"
#include "file.pb.h"

#include <string>
#include <tuple>

#include "UploadSession.hpp"

class FTPServiceImpl final : public FTPService::Service
{
public:
        FTPServiceImpl(const std::string_view root_dir);

public:
        bool IsValid() const noexcept;

private:
        grpc::Status UploadFile(grpc::ServerContext* context, grpc::ServerReader<UploadFileRequest>* reader, UploadFileResponse* response) override;

private:
	std::tuple<bool, UploadSession, grpc::Status> OpenFile(grpc::ServerReader<UploadFileRequest>* reader) noexcept;
	std::tuple<bool, grpc::Status> WriteToFile(grpc::ServerReader<UploadFileRequest>* reader, UploadSession& session) noexcept;
	std::tuple<bool, FileMetaData, grpc::Status> CheckHash(grpc::ServerReader<UploadFileRequest>* reader, const UploadSession& session) noexcept;
	
private:
        const std::string root_dir_;
};

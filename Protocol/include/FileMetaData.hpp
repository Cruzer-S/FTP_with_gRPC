#pragma once

#include "protocol.pb.h"

#include <filesystem>
#include <string>

FileMetaData MakeFileMetaDataFrom(const std::filesystem::path& from, std::string hash);
std::string FileMetaDataToString(const FileMetaData& metadata);
#pragma once

#include "file.pb.h"

#include <filesystem>

FileMetaData MakeFileMetaDataFrom(const std::filesystem::path& from);

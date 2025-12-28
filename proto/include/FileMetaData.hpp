#pragma once

#include "File.pb.h"

#include <filesystem>

FileMetaData MakeFileMetaDataFrom(const std::filesystem::path& from);

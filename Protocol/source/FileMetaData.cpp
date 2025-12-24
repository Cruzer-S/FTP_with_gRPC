#include "FileMetaData.hpp"

#include <filesystem>
#include <iostream>
#include <iomanip>
#include <string>

#include <sys/stat.h>

#include <google/protobuf/util/time_util.h>
#include <google/protobuf/timestamp.pb.h>

namespace {
    google::protobuf::Timestamp TimespecToTimestamp(const struct timespec& ts)
    {
        google::protobuf::Timestamp out;

        out.set_seconds(ts.tv_sec);
        out.set_nanos(ts.tv_nsec);

        return out;
    }

    enum class FileTimeKind {
        Atime,
        Mtime,
        Ctime
    };

    google::protobuf::Timestamp PathToTimestamp(const std::string_view path, FileTimeKind kind) {
        struct stat st;

        stat(path.data(), &st);

        switch (kind) {
        case FileTimeKind::Atime:
            return TimespecToTimestamp(st.st_atim);
        case FileTimeKind::Mtime:
            return TimespecToTimestamp(st.st_mtim);
        case FileTimeKind::Ctime:
            return TimespecToTimestamp(st.st_ctim);
        }

        return {}; // unreachable
    }
}

FileMetaData MakeFileMetaDataFrom(const std::filesystem::path& from, std::string hash)
{
    FileMetaData data;

    data.set_path(from);
    data.set_size(std::filesystem::file_size(from));
    data.set_hash(hash);

    data.mutable_create_time()->CopyFrom(PathToTimestamp(from.c_str(), FileTimeKind::Ctime));
    data.mutable_modify_time()->CopyFrom(PathToTimestamp(from.c_str(), FileTimeKind::Mtime));
    data.mutable_access_time()->CopyFrom(PathToTimestamp(from.c_str(), FileTimeKind::Atime));

    return data;
}

std::string FileMetaDataToString(const FileMetaData& metadata)
{
    using google::protobuf::util::TimeUtil;

    std::stringstream ss;

    ss << "path: " << metadata.path() << std::endl;
    ss << "size: " << metadata.size() << std::endl;
    ss << std::uppercase << std::hex << std::setfill('0');
    for (unsigned char byte : metadata.hash())
        ss << std::setw(2) << static_cast<int>(byte) << ' ';
    ss << std::dec << std::endl;
    ss << "ctime: " << TimeUtil::ToString(metadata.create_time()) << std::endl;
    ss << "mtime: " << TimeUtil::ToString(metadata.modify_time()) << std::endl;
    ss << "atime: " << TimeUtil::ToString(metadata.access_time()) << std::endl;

    return ss.str();
}
#include "FileMetaData.hpp"

#include <filesystem>

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
        Atime, Mtime, Ctime
    };

    google::protobuf::Timestamp PathToTimestamp(const std::filesystem::path &path, FileTimeKind kind) {
        struct stat st;

        if (stat(path.c_str(), &st) != 0)
		return {};

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

FileMetaData MakeFileMetaDataFrom(const std::filesystem::path& from)
{
    FileMetaData data;

    data.set_path(from);
    data.set_size(std::filesystem::file_size(from));

    data.mutable_create_time()->CopyFrom(PathToTimestamp(from, FileTimeKind::Ctime));
    data.mutable_modify_time()->CopyFrom(PathToTimestamp(from, FileTimeKind::Mtime));
    data.mutable_access_time()->CopyFrom(PathToTimestamp(from, FileTimeKind::Atime));

    return data;
}

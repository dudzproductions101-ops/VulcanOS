


































#ifndef VULCAN_PKG_VPK_ARCHIVE_H
#define VULCAN_PKG_VPK_ARCHIVE_H

#include "types.h"

#define VPK_MAGIC "VULCPKG\0"
#define VPK_MAGIC_LEN 8
#define VPK_FORMAT_VERSION 1

#define VPK_MAX_ENTRIES 64
#define VPK_MAX_PATH 128

struct vpk_entry {
    char path[VPK_MAX_PATH];
    const u8 *content;    


    u64 content_len;
};

struct vpk_archive {
    u32 format_version;
    u32 entry_count;
    struct vpk_entry entries[VPK_MAX_ENTRIES];
};

















bool vpk_parse(const u8 *data, usize size, struct vpk_archive *out);





const struct vpk_entry *vpk_find(const struct vpk_archive *archive, const char *path);

#endif 

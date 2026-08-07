












#include "pkg/vpk_archive.h"






static u16 read_u16le(const u8 *p)
{
    return (u16)p[0] | ((u16)p[1] << 8);
}

static u32 read_u32le(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static u64 read_u64le(const u8 *p)
{
    u64 result = 0;
    for (int i = 7; i >= 0; i--) {
        result = (result << 8) | p[i];
    }
    return result;
}

bool vpk_parse(const u8 *data, usize size, struct vpk_archive *out)
{
    if (size < VPK_MAGIC_LEN + 4 + 4) {
        return false; 
    }

    for (int i = 0; i < VPK_MAGIC_LEN; i++) {
        if (data[i] != VPK_MAGIC[i]) {
            return false;
        }
    }

    usize offset = VPK_MAGIC_LEN;

    out->format_version = read_u32le(data + offset);
    offset += 4;
    if (out->format_version != VPK_FORMAT_VERSION) {
        return false; 


    }

    out->entry_count = read_u32le(data + offset);
    offset += 4;
    if (out->entry_count > VPK_MAX_ENTRIES) {
        return false;
    }

    for (u32 i = 0; i < out->entry_count; i++) {
        if (offset + 2 > size) {
            return false;
        }
        u16 path_len = read_u16le(data + offset);
        offset += 2;

        if (path_len >= VPK_MAX_PATH || path_len > size - offset) {
            return false;
        }
        for (u16 j = 0; j < path_len; j++) {
            out->entries[i].path[j] = (char)data[offset + j];
        }
        out->entries[i].path[path_len] = '\0';
        offset += path_len;

        if (offset + 8 > size) {
            return false;
        }
        u64 content_len = read_u64le(data + offset);
        offset += 8;

        

















        if (content_len > size - offset) {
            return false; 


        }
        out->entries[i].content = data + offset;
        out->entries[i].content_len = content_len;
        offset += content_len;
    }

    return true;
}

const struct vpk_entry *vpk_find(const struct vpk_archive *archive, const char *path)
{
    for (u32 i = 0; i < archive->entry_count; i++) {
        const char *a = archive->entries[i].path;
        const char *b = path;
        while (*a && *a == *b) {
            a++;
            b++;
        }
        if (*a == *b) { 
            return &archive->entries[i];
        }
    }
    return NULL;
}

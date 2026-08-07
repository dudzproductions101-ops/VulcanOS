/*
 * vpk_archive.h - .vpk archive format: reader
 *
 * VulcanOS's own package archive format, designed from scratch for
 * this project (not tar, not zip, not cpio) -- a small, original
 * format was the right choice here because every general-purpose
 * archive format brings substantial complexity (tar's multiple
 * historical header variants, zip's central-directory-plus-local-
 * headers redundancy, both often with size/alignment/padding rules
 * that exist for reasons irrelevant to VulcanOS) that this bring-up
 * milestone's actual requirement -- "a named list of files with
 * their raw contents" -- does not need.
 *
 * WIRE FORMAT (all integers little-endian, matching x86_64's native
 * byte order so no byte-swapping is needed on this platform):
 *
 *   offset  size  field
 *   0       8     magic: the ASCII bytes "VULCPKG\0"
 *   8       4     format_version (currently 1)
 *   12      4     entry_count (N)
 *   16      ...   N repeated entries, each:
 *                    2   path_len (L)
 *                    L   path bytes (NOT NUL-terminated in the
 *                        file; the reader NUL-terminates into its
 *                        own buffer)
 *                    8   content_len (C)
 *                    C   raw content bytes
 *
 * Every length is explicit and read before the data it describes,
 * so a reader can validate declared sizes against what's actually
 * present before trusting any of it, and never needs to scan for a
 * delimiter. By convention (not a wire-format requirement -- vpkg
 * itself enforces this), entry 0 is always "manifest.vconf".
 */

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
    const u8 *content;    /* points INTO the original archive buffer --
                            * see vpk_parse's contract below for the
                            * lifetime this requires */
    u64 content_len;
};

struct vpk_archive {
    u32 format_version;
    u32 entry_count;
    struct vpk_entry entries[VPK_MAX_ENTRIES];
};

/* Parses `data` (an in-memory .vpk archive, `size` bytes) into
 * `out`. Returns true on success. `out`'s entries' `content`
 * pointers point directly into `data` -- they are NOT copied -- so
 * `data` must remain valid and unmodified for as long as `out` is
 * used. This is a deliberate zero-copy design: packages this bring-
 * up milestone installs are compiled directly into the kernel image
 * as embedded byte arrays (see pkg/embedded_packages.c) with
 * `static const` storage duration, so "must remain valid" is
 * trivially satisfied by construction -- copying would just be
 * wasted work and wasted heap.
 *
 * Validates the magic, format version, and that every entry's
 * declared path_len/content_len actually fits within `size` before
 * returning true -- a corrupt or truncated archive is rejected here
 * rather than causing an out-of-bounds read later when some entry's
 * content is actually used. */
bool vpk_parse(const u8 *data, usize size, struct vpk_archive *out);

/* Finds the entry with the given path within an already-parsed
 * archive, or NULL if no such entry exists. Linear search --
 * VPK_MAX_ENTRIES (64) is small enough that this is not a
 * meaningful cost at this scale. */
const struct vpk_entry *vpk_find(const struct vpk_archive *archive, const char *path);

#endif /* VULCAN_PKG_VPK_ARCHIVE_H */

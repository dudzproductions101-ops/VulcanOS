/*
 * vpk_archive.c - .vpk archive format: reader implementation
 *
 * See pkg/vpk_archive.h for the wire format. This parser is
 * deliberately defensive -- checking every declared length against
 * remaining buffer size before trusting it -- even though every
 * current caller only ever parses trusted, compiled-in data (see
 * vpk_parse's own zero-copy design note in the header). Writing it
 * defensively now, rather than "we control the input so it's fine,"
 * is what makes this format genuinely reusable once a real transport
 * (network, removable media) can hand vpkg untrusted bytes.
 */

#include "pkg/vpk_archive.h"

/* Little-endian reads, explicit rather than a cast-and-dereference,
 * so this parser's correctness doesn't depend on the host compiler
 * happening to also be little-endian (x86_64 always is, but writing
 * it out explicitly documents the wire format's own endianness
 * rather than silently inheriting the platform's). */
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
        return false; /* too small to even hold the fixed header */
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
        return false; /* unknown format version -- reject rather than
                        * guess at a layout this reader was never
                        * written to understand */
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

        /* SECURITY-RELEVANT CHECK: written as `content_len > size -
         * offset` rather than the more obvious-looking `offset +
         * content_len > size`. The latter is a real integer-overflow
         * bug: content_len is attacker-controlled (read directly
         * from the archive), and if it's near UINT64_MAX, offset +
         * content_len wraps around (unsigned overflow is well-
         * defined modulo 2^64 in C, not undefined behavior, but
         * still produces a small wrapped value here) to something
         * that incorrectly passes the check -- verified numerically
         * before this fix shipped: offset=100, content_len close to
         * UINT64_MAX wrapped the sum to 49, which is <= a size of
         * 1000 and would have been wrongly accepted. `size - offset`
         * cannot overflow here (offset <= size is already an
         * established loop invariant at this point -- every prior
         * offset advance was itself bounds-checked), so comparing
         * content_len against that remaining-space value directly
         * is safe regardless of how large content_len claims to
         * be. */
        if (content_len > size - offset) {
            return false; /* declared content length would read past
                            * the end of the buffer -- reject rather
                            * than read out of bounds */
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
        if (*a == *b) { /* both reached NUL simultaneously */
            return &archive->entries[i];
        }
    }
    return NULL;
}

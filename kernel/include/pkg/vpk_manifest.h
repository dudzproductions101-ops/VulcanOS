/*
 * vpk_manifest.h - manifest.vconf parser
 *
 * Every .vpk archive's entry 0 is, by vpkg's own convention, a text
 * file named "manifest.vconf" in this format:
 *
 *   name: hello-vulcan
 *   version: 1.0.0
 *   description: A minimal example VulcanOS package
 *   depends:
 *
 *   [files]
 *   share/hello.txt -> /packages/hello-vulcan/hello.txt
 *   config/hello.conf -> /config/hello-vulcan.conf
 *
 * Line-oriented key: value pairs, then a [files] section listing
 * SOURCE -> DEST mappings (source is a path within the archive;
 * dest is the absolute VulcanOS path to install it to). Chosen over
 * JSON/YAML deliberately: this project would have to write a real
 * JSON/YAML parser from scratch to use either (there is no host
 * library to link against in a freestanding kernel build), and a
 * general nested-structure parser is a much bigger undertaking than
 * this format's actual needs -- a flat key:value block plus one
 * list section. This format is also, deliberately, easy to hand-
 * write without any tooling at all: no escaping rules beyond "don't
 * put a newline in a value," no nesting, no quoting.
 */

#ifndef VULCAN_PKG_VPK_MANIFEST_H
#define VULCAN_PKG_VPK_MANIFEST_H

#include "types.h"
#include "pkg/vpk_archive.h" /* for VPK_MAX_PATH, used by
                               * vpk_file_mapping.source below */

#define VPK_MANIFEST_FILE "manifest.vconf"

#define VPK_NAME_MAX 64
#define VPK_VERSION_MAX 32
#define VPK_DESC_MAX 128
#define VPK_MAX_DEPS 8
#define VPK_MAX_FILE_MAPPINGS 32

struct vpk_file_mapping {
    char source[VPK_MAX_PATH];       /* path within the archive */
    char dest[256];                   /* absolute VulcanOS install path;
                                        * matches VULCAN_PATH_MAX from
                                        * fs/vfs.h, duplicated as a
                                        * literal so this header doesn't
                                        * need to depend on fs/vfs.h just
                                        * for one constant */
};

struct vpk_manifest {
    char name[VPK_NAME_MAX];
    char version[VPK_VERSION_MAX];
    char description[VPK_DESC_MAX];

    char depends[VPK_MAX_DEPS][VPK_NAME_MAX];
    u32 depends_count;

    struct vpk_file_mapping files[VPK_MAX_FILE_MAPPINGS];
    u32 files_count;
};

/* Parses manifest text (NUL-terminated, `len` bytes not counting the
 * NUL) into `out`. Returns true if at least `name` and `version`
 * were present -- every other field is optional and left as an
 * empty string / zero count if absent, matching this format's
 * deliberately permissive, hand-writable design. */
bool vpk_manifest_parse(const char *text, usize len, struct vpk_manifest *out);

#endif /* VULCAN_PKG_VPK_MANIFEST_H */

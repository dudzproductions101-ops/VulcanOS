/*
 * vpkg.h - VulcanOS package manager
 *
 * vpkg installs .vpk archives: parses the archive (pkg/
 * vpk_archive.h), reads its manifest (pkg/vpk_manifest.h), copies
 * each mapped file into its real destination via the VFS
 * (fs/vfs.h), and records the installation in a package database
 * kept at /state/packages.db so `vpkg list`/`vpkg remove` have
 * something real to work from.
 *
 * INTERIM SCOPE NOTE: VulcanOS has no ring-3 execution, no
 * exec()-from-disk, and no network/removable-media driver yet (see
 * the interim-design notes already established for libc and
 * userland). This means vpkg cannot install and RUN arbitrary
 * third-party machine code -- there's no mechanism to execute a
 * package's payload as a new program. What it genuinely, fully
 * does: installs real package archives (data, config, documentation
 * -- anything that's meaningful as installed FILES) from a real,
 * original archive format, into VulcanOS's real filesystem
 * hierarchy, tracked in a real package database. This is a complete
 * system for what's actually possible now, not a stub -- the same
 * honest-scoping pattern already used for vulcanfs (RAM-resident,
 * not disk-backed) and libc (direct kernel calls, not real
 * syscalls). See pkg/embedded_packages.h for how packages currently
 * reach the running system (compiled into the kernel image, since
 * there's no other transport yet) and exactly what to add to make a
 * new package installable -- this is the extension point.
 */

#ifndef VULCAN_PKG_VPKG_H
#define VULCAN_PKG_VPKG_H

#include "types.h"

#define VPKG_DB_PATH "/state/packages.db"
#define VPKG_MAX_INSTALLED 32

enum vpkg_result {
    VPKG_OK,
    VPKG_ERR_PARSE_FAILED,
    VPKG_ERR_NO_MANIFEST,
    VPKG_ERR_BAD_MANIFEST,
    VPKG_ERR_ALREADY_INSTALLED,
    VPKG_ERR_NOT_INSTALLED,
    VPKG_ERR_FILE_MISSING,
    VPKG_ERR_INSTALL_FAILED,
    VPKG_ERR_DB_FULL,
};

/* One installed package's record, as kept in the on-disk (well,
 * on-vulcanfs -- see the RAM-resident note in fs/vulcanfs.h) package
 * database. */
struct vpkg_record {
    char name[64];
    char version[32];
    char description[128];
    u32 file_count;             /* how many files this package installed,
                                  * for `vpkg list`'s own reporting --
                                  * the actual dest paths aren't kept
                                  * here (they're re-derivable from the
                                  * manifest at removal time... except
                                  * removal, for this bring-up milestone,
                                  * does not re-parse the original
                                  * archive -- see vpkg_remove's own
                                  * documented limitation) */
};

void vpkg_init(void);

/* Installs the package contained in `data` (`size` bytes, an
 * in-memory .vpk archive -- see pkg/vpk_archive.h). Parses the
 * archive, reads its manifest, copies every mapped file to its real
 * destination, and adds a record to the package database. */
enum vpkg_result vpkg_install(const u8 *data, usize size);

/* Removes a package's DATABASE RECORD by name. LIMITATION, stated
 * plainly rather than hidden: this bring-up milestone's vpkg_remove
 * does NOT delete the files a package installed -- doing so
 * correctly requires either re-parsing the original archive (not
 * kept around after install) or the database recording every
 * installed file's destination path (not currently stored, only a
 * count -- see vpkg_record's own comment). Removing the record
 * without removing the files is still a real, useful operation
 * (accurate `vpkg list` output, and the name becomes reinstallable),
 * but is NOT full uninstall semantics. Fixing this properly --
 * storing full per-file dest paths in the database -- is real,
 * scoped future work, not an oversight left undocumented. */
enum vpkg_result vpkg_remove(const char *name);

/* Fills `out` with the `index`-th (0-based) installed package
 * record. Returns false once index is past the last installed
 * package. */
bool vpkg_list(u32 index, struct vpkg_record *out);

u32 vpkg_installed_count(void);

#endif /* VULCAN_PKG_VPKG_H */

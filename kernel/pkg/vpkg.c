


















#include "pkg/vpkg.h"
#include "pkg/vpk_archive.h"
#include "pkg/vpk_manifest.h"
#include "fs/vfs.h"
#include "printk.h"
#include "panic.h"

static struct vpkg_record installed[VPKG_MAX_INSTALLED];
static u32 installed_count = 0;

static bool names_equal(const char *a, const char *b)
{
    while (*a && *b) {
        if (*a != *b) {
            return false;
        }
        a++;
        b++;
    }
    return *a == *b;
}





static void copy_bounded(char *dest, usize dest_size, const char *src)
{
    usize i = 0;
    for (; i < dest_size - 1 && src[i]; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

static void append_str(char *buf, usize buf_size, usize *pos, const char *s)
{
    while (*s && *pos < buf_size - 1) {
        buf[(*pos)++] = *s;
    }
}

static void append_u32(char *buf, usize buf_size, usize *pos, u32 value)
{
    char tmp[12];
    int i = 0;
    if (value == 0) {
        tmp[i++] = '0';
    }
    while (value > 0) {
        tmp[i++] = (char)('0' + (value % 10));
        value /= 10;
    }
    while (i > 0 && *pos < buf_size - 1) {
        buf[(*pos)++] = tmp[--i];
    }
}







static void persist_db(void)
{
    char buf[4096];
    usize pos = 0;

    for (u32 i = 0; i < installed_count; i++) {
        append_str(buf, sizeof(buf), &pos, installed[i].name);
        append_str(buf, sizeof(buf), &pos, "|");
        append_str(buf, sizeof(buf), &pos, installed[i].version);
        append_str(buf, sizeof(buf), &pos, "|");
        append_str(buf, sizeof(buf), &pos, installed[i].description);
        append_str(buf, sizeof(buf), &pos, "|");
        append_u32(buf, sizeof(buf), &pos, installed[i].file_count);
        append_str(buf, sizeof(buf), &pos, "\n");
    }

    vfs_unlink(VPKG_DB_PATH); 



    vfs_create(VPKG_DB_PATH, INODE_FILE);
    vulcan_fd_t fd = vfs_open(VPKG_DB_PATH);
    if (fd != VULCAN_FD_INVALID) {
        vfs_write(fd, buf, pos);
        vfs_close(fd);
    }
}

void vpkg_init(void)
{
    installed_count = 0;
    



    printk_level(LOG_INFO, "vpkg: initialized (db at %s)\n", VPKG_DB_PATH);
}

static bool find_installed_index(const char *name, u32 *out_index)
{
    for (u32 i = 0; i < installed_count; i++) {
        if (names_equal(installed[i].name, name)) {
            *out_index = i;
            return true;
        }
    }
    return false;
}































static void ensure_parent_dirs_exist(const char *path)
{
    static u32 call_count = 0;
    call_count++;
    printk_level(LOG_DEBUG, "vpkg: ensure_parent_dirs_exist call #%u for \"%s\"\n",
                 call_count, path);
    if (call_count > 20) {
        panic("vpkg: ensure_parent_dirs_exist called >20 times -- likely infinite loop upstream");
    }

    char full_path[VULCAN_PATH_MAX];
    usize full_len = 0;
    char component[VULCAN_PATH_MAX];
    usize comp_len = 0;

    const char *p = path;
    if (*p == '/') {
        p++;
    }

    for (;; p++) {
        if (*p == '/' || *p == '\0') {
            if (comp_len == 0) {
                if (*p == '\0') {
                    break; 
                }
                continue; 
            }

            if (*p == '\0') {
                

                break;
            }

            


            if (full_len < sizeof(full_path) - 1) {
                full_path[full_len++] = '/';
            }
            for (usize i = 0; i < comp_len && full_len < sizeof(full_path) - 1; i++) {
                full_path[full_len++] = component[i];
            }
            full_path[full_len] = '\0';

            









            bool created = vfs_create(full_path, INODE_DIRECTORY);
            printk_level(LOG_DEBUG, "vpkg: mkdir %s: %s\n",
                         full_path, created ? "created" : "already exists");

            comp_len = 0;
            continue;
        }

        if (comp_len < sizeof(component) - 1) {
            component[comp_len++] = *p;
        }
    }
}









static bool install_one_file(const struct vpk_entry *entry, const char *dest_path)
{
    printk_level(LOG_DEBUG, "install_one_file: ENTER dest=\"%s\" content_len=%llu\n",
                 dest_path, (u64)entry->content_len);

    ensure_parent_dirs_exist(dest_path);

    bool created = vfs_create(dest_path, INODE_FILE);
    printk_level(LOG_DEBUG, "vpkg: create %s: %s\n",
                 dest_path, created ? "created" : "already exists or parent missing");

    vulcan_fd_t fd = vfs_open(dest_path);
    printk_level(LOG_DEBUG, "install_one_file: vfs_open(\"%s\") returned fd=%d\n", dest_path, fd);
    if (fd == VULCAN_FD_INVALID) {
        printk_level(LOG_ERROR, "vpkg: open failed for %s (parent directory missing?)\n",
                     dest_path);
        return false;
    }

    isize written = vfs_write(fd, entry->content, entry->content_len);
    vfs_close(fd);

    printk_level(LOG_DEBUG, "vpkg: write %s: %lld/%llu bytes\n",
                 dest_path, (i64)written, (u64)entry->content_len);

    return written == (isize)entry->content_len;
}

enum vpkg_result vpkg_install(const u8 *data, usize size)
{
    printk_level(LOG_DEBUG, "vpkg_install: ENTER, data=%p size=%llu\n", (void *)data, (u64)size);

    struct vpk_archive archive;
    printk_level(LOG_DEBUG, "vpkg_install: calling vpk_parse...\n");
    if (!vpk_parse(data, size, &archive)) {
        printk_level(LOG_DEBUG, "vpkg_install: vpk_parse FAILED\n");
        return VPKG_ERR_PARSE_FAILED;
    }
    printk_level(LOG_DEBUG, "vpkg_install: vpk_parse OK, entry_count=%u\n", archive.entry_count);

    printk_level(LOG_DEBUG, "vpkg_install: calling vpk_find for manifest...\n");
    const struct vpk_entry *manifest_entry = vpk_find(&archive, VPK_MANIFEST_FILE);
    if (!manifest_entry) {
        printk_level(LOG_DEBUG, "vpkg_install: manifest entry NOT FOUND\n");
        return VPKG_ERR_NO_MANIFEST;
    }
    printk_level(LOG_DEBUG, "vpkg_install: manifest found, content_len=%llu\n",
                 (u64)manifest_entry->content_len);

    struct vpk_manifest manifest;
    printk_level(LOG_DEBUG, "vpkg_install: calling vpk_manifest_parse...\n");
    if (!vpk_manifest_parse((const char *)manifest_entry->content,
                             (usize)manifest_entry->content_len, &manifest)) {
        printk_level(LOG_DEBUG, "vpkg_install: vpk_manifest_parse FAILED\n");
        return VPKG_ERR_BAD_MANIFEST;
    }
    printk_level(LOG_DEBUG, "vpkg_install: manifest parsed OK: name=\"%s\" version=\"%s\" "
                 "files_count=%u depends_count=%u\n",
                 manifest.name, manifest.version, manifest.files_count, manifest.depends_count);

    u32 existing_index;
    printk_level(LOG_DEBUG, "vpkg_install: checking find_installed_index...\n");
    if (find_installed_index(manifest.name, &existing_index)) {
        printk_level(LOG_DEBUG, "vpkg_install: already installed at index %u\n", existing_index);
        return VPKG_ERR_ALREADY_INSTALLED;
    }
    printk_level(LOG_DEBUG, "vpkg_install: not already installed, proceeding\n");

    if (installed_count >= VPKG_MAX_INSTALLED) {
        printk_level(LOG_DEBUG, "vpkg_install: database full (installed_count=%u)\n", installed_count);
        return VPKG_ERR_DB_FULL;
    }

    printk_level(LOG_INFO, "vpkg: installing %s %s (%u files)\n",
                 manifest.name, manifest.version, manifest.files_count);

    u32 files_installed = 0;
    for (u32 i = 0; i < manifest.files_count; i++) {
        printk_level(LOG_DEBUG, "vpkg_install: file loop iteration i=%u/%u\n",
                     i, manifest.files_count);

        const struct vpk_file_mapping *fm = &manifest.files[i];
        printk_level(LOG_DEBUG, "vpkg_install: mapping[%u] source=\"%s\" dest=\"%s\"\n",
                     i, fm->source, fm->dest);

        const struct vpk_entry *entry = vpk_find(&archive, fm->source);

        if (!entry) {
            printk_level(LOG_WARN, "vpkg: manifest references missing archive entry \"%s\"\n",
                         fm->source);
            return VPKG_ERR_FILE_MISSING;
        }
        printk_level(LOG_DEBUG, "vpkg_install: found archive entry for \"%s\", content_len=%llu\n",
                     fm->source, (u64)entry->content_len);

        printk_level(LOG_DEBUG, "vpkg_install: calling install_one_file for \"%s\"...\n", fm->dest);
        bool ok = install_one_file(entry, fm->dest);
        printk_level(LOG_DEBUG, "vpkg_install: install_one_file(\"%s\") returned %s\n",
                     fm->dest, ok ? "true" : "false");

        if (!ok) {
            printk_level(LOG_ERROR, "vpkg: failed to install \"%s\" -> \"%s\"\n",
                         fm->source, fm->dest);
            return VPKG_ERR_INSTALL_FAILED;
        }

        files_installed++;
    }
    printk_level(LOG_DEBUG, "vpkg_install: file loop complete, files_installed=%u\n", files_installed);

    struct vpkg_record *rec = &installed[installed_count];
    copy_bounded(rec->name, sizeof(rec->name), manifest.name);
    copy_bounded(rec->version, sizeof(rec->version), manifest.version);
    copy_bounded(rec->description, sizeof(rec->description), manifest.description);
    rec->file_count = files_installed;
    installed_count++;

    persist_db();

    printk_level(LOG_INFO, "vpkg: %s %s installed successfully (%u/%u files)\n",
                 manifest.name, manifest.version, files_installed, manifest.files_count);

    return VPKG_OK;
}

enum vpkg_result vpkg_remove(const char *name)
{
    u32 index;
    if (!find_installed_index(name, &index)) {
        return VPKG_ERR_NOT_INSTALLED;
    }

    


    for (u32 i = index; i < installed_count - 1; i++) {
        installed[i] = installed[i + 1];
    }
    installed_count--;

    persist_db();

    printk_level(LOG_INFO, "vpkg: removed %s from the package database "
                 "(installed files were NOT deleted -- see vpkg_remove's documented limitation)\n",
                 name);

    return VPKG_OK;
}

bool vpkg_list(u32 index, struct vpkg_record *out)
{
    if (index >= installed_count) {
        return false;
    }
    *out = installed[index];
    return true;
}

u32 vpkg_installed_count(void)
{
    return installed_count;
}

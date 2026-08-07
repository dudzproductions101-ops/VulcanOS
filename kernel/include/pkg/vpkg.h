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

struct vpkg_record {
    char name[64];
    char version[32];
    char description[128];
    u32 file_count;             

};

void vpkg_init(void);

enum vpkg_result vpkg_install(const u8 *data, usize size);

enum vpkg_result vpkg_remove(const char *name);

bool vpkg_list(u32 index, struct vpkg_record *out);

u32 vpkg_installed_count(void);

#endif 

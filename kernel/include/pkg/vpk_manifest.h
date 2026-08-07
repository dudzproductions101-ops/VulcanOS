



























#ifndef VULCAN_PKG_VPK_MANIFEST_H
#define VULCAN_PKG_VPK_MANIFEST_H

#include "types.h"
#include "pkg/vpk_archive.h" 


#define VPK_MANIFEST_FILE "manifest.vconf"

#define VPK_NAME_MAX 64
#define VPK_VERSION_MAX 32
#define VPK_DESC_MAX 128
#define VPK_MAX_DEPS 8
#define VPK_MAX_FILE_MAPPINGS 32

struct vpk_file_mapping {
    char source[VPK_MAX_PATH];       
    char dest[256];                   





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






bool vpk_manifest_parse(const char *text, usize len, struct vpk_manifest *out);

#endif 

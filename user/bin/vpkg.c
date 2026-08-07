






















#include "stdio.h"
#include "string.h"
#include "pkg/vpkg.h"
#include "pkg/embedded_packages.h"

static void print_usage(void)
{
    printf("usage: vpkg install <name>   install an embedded package\n");
    printf("       vpkg list              list installed packages\n");
    printf("       vpkg available          list packages available to install\n");
    printf("       vpkg remove <name>     remove a package's database record\n");
}

static int cmd_install(int argc, char **argv)
{
    if (argc < 3) {
        printf("vpkg: install requires a package name\n");
        return 1;
    }

    const char *name = argv[2];
    const struct embedded_package *pkg = embedded_package_find(name);
    if (!pkg) {
        printf("vpkg: no embedded package named \"%s\" "
               "(try 'vpkg available' to see what's installable)\n", name);
        return 1;
    }

    enum vpkg_result result = vpkg_install(pkg->data, pkg->size);

    switch (result) {
    case VPKG_OK:
        printf("vpkg: %s installed successfully\n", name);
        return 0;
    case VPKG_ERR_ALREADY_INSTALLED:
        printf("vpkg: %s is already installed\n", name);
        return 1;
    case VPKG_ERR_PARSE_FAILED:
        printf("vpkg: %s's archive failed to parse (corrupt .vpk?)\n", name);
        return 1;
    case VPKG_ERR_NO_MANIFEST:
        printf("vpkg: %s has no manifest.vconf\n", name);
        return 1;
    case VPKG_ERR_BAD_MANIFEST:
        printf("vpkg: %s's manifest is missing required fields (name/version)\n", name);
        return 1;
    case VPKG_ERR_FILE_MISSING:
        printf("vpkg: %s's manifest references a file not present in the archive\n", name);
        return 1;
    case VPKG_ERR_INSTALL_FAILED:
        printf("vpkg: %s failed while copying a file to its destination\n", name);
        return 1;
    case VPKG_ERR_DB_FULL:
        printf("vpkg: package database is full\n");
        return 1;
    default:
        printf("vpkg: install failed (unknown error)\n");
        return 1;
    }
}

static int cmd_list(void)
{
    u32 count = vpkg_installed_count();
    if (count == 0) {
        printf("vpkg: no packages installed\n");
        return 0;
    }

    struct vpkg_record rec;
    for (u32 i = 0; vpkg_list(i, &rec); i++) {
        printf("%s %s - %s (%u files)\n", rec.name, rec.version, rec.description, rec.file_count);
    }
    return 0;
}

static int cmd_available(void)
{
    u32 count = embedded_package_count();
    if (count == 0) {
        printf("vpkg: no packages available\n");
        return 0;
    }

    for (u32 i = 0; i < count; i++) {
        const struct embedded_package *pkg = embedded_package_at(i);
        printf("%s\n", pkg->install_name);
    }
    return 0;
}

static int cmd_remove(int argc, char **argv)
{
    if (argc < 3) {
        printf("vpkg: remove requires a package name\n");
        return 1;
    }

    enum vpkg_result result = vpkg_remove(argv[2]);
    if (result == VPKG_ERR_NOT_INSTALLED) {
        printf("vpkg: %s is not installed\n", argv[2]);
        return 1;
    }

    printf("vpkg: %s removed from the package database\n", argv[2]);
    printf("      (note: installed files were NOT deleted -- vpkg_remove's\n");
    printf("       current limitation; see kernel/include/pkg/vpkg.h)\n");
    return 0;
}

int vpkg_main(int argc, char **argv)
{
    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "install") == 0) {
        return cmd_install(argc, argv);
    }
    if (strcmp(argv[1], "list") == 0) {
        return cmd_list();
    }
    if (strcmp(argv[1], "available") == 0) {
        return cmd_available();
    }
    if (strcmp(argv[1], "remove") == 0) {
        return cmd_remove(argc, argv);
    }

    printf("vpkg: unknown subcommand \"%s\"\n", argv[1]);
    print_usage();
    return 1;
}

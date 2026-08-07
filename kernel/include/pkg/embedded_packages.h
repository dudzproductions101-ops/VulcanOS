/*
 * embedded_packages.h - How .vpk packages currently reach VulcanOS
 *
 * ============================================================
 * THIS IS THE FILE TO EDIT TO ADD A NEW PACKAGE. Nothing else in
 * vpkg's own source needs to change.
 * ============================================================
 *
 * VulcanOS has no network stack and no removable-media driver yet
 * (see vpkg.h's interim-scope note), so a .vpk archive's only
 * current path onto a running system is: build it on the host with
 * tools/vpkbuild.py, turn its bytes into a C byte array with
 * tools/bin2c.py, and compile that array directly into the kernel
 * image. This file is the registry of every such embedded package.
 *
 * TO ADD A NEW PACKAGE:
 *   1. Create a package source directory anywhere convenient, e.g.
 *      tools/packages/my-package/ with a manifest.vconf and
 *      whatever files it installs (see tools/packages/hello-vulcan/
 *      for a real, working example).
 *   2. Run: python3 tools/vpkbuild.py tools/packages/my-package
 *      my-package.vpk
 *      This produces my-package.vpk (a real .vpk archive, see
 *      pkg/vpk_archive.h for the format).
 *   3. Run: python3 tools/bin2c.py my-package.vpk
 *      kernel/pkg/embedded_my_package.c g_pkg_my_package
 *      This generates a .c file with your archive's bytes as a
 *      `static const u8[]` array plus a matching `usize` length
 *      variable, ready to compile straight into the kernel.
 *   4. Add ONE extern declaration + ONE registry entry below.
 *   5. Rebuild. Your package is now installable via `vpkg install
 *      my-package` from vulsh.
 *
 * No vpkg source file needs to change for any of this -- that's the
 * actual point of this registry existing as its own small file.
 */

#ifndef VULCAN_PKG_EMBEDDED_PACKAGES_H
#define VULCAN_PKG_EMBEDDED_PACKAGES_H

#include "types.h"

struct embedded_package {
    const char *install_name;   /* what `vpkg install <name>` matches against --
                                  * does not have to equal the manifest's own
                                  * `name:` field, though keeping them identical
                                  * is the sane default and what the generator
                                  * scripts produce by default */
    const u8 *data;
    usize size;
};

/* Returns the embedded package registered under `install_name`, or
 * NULL if no such package is registered -- see
 * pkg/embedded_packages.c for the actual registry array this
 * searches, which is the only other file a new package addition
 * touches. */
const struct embedded_package *embedded_package_find(const char *install_name);

/* Returns the total number of registered embedded packages, and
 * fills `out` with the `index`-th one -- used by `vpkg available`
 * (vulsh's package-listing command) to show what CAN be installed,
 * not just what already IS. */
u32 embedded_package_count(void);
const struct embedded_package *embedded_package_at(u32 index);

#endif /* VULCAN_PKG_EMBEDDED_PACKAGES_H */

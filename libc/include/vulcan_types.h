/*
 * vulcan_types.h - Standard C type names for VulcanOS libc consumers
 *
 * Userland code should be able to write ordinary, standard-looking C
 * (size_t, NULL, ...) without needing to know VulcanOS's own
 * internal type names (usize, u8, ...) from kernel/include/types.h.
 * This header is the bridge: every standard name here is defined
 * directly in terms of VulcanOS's own types, so there is exactly
 * ONE real definition of "how wide is a pointer-sized integer on
 * this platform" (types.h), not two definitions that could quietly
 * drift out of sync with each other.
 *
 * This is NOT <stddef.h> or any other hosted-libc header -- it is
 * VulcanOS's own file, and it depends only on VulcanOS's own
 * types.h, never on a host system's headers. A build of VulcanOS's
 * libc never reaches out to whatever C library happens to be
 * installed on the machine compiling it.
 */

#ifndef VULCAN_LIBC_TYPES_H
#define VULCAN_LIBC_TYPES_H

#include "types.h"  /* kernel/include/types.h, on the include path --
                      * see libc/Makefile's -I flags */

typedef usize size_t;
typedef isize ssize_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#endif /* VULCAN_LIBC_TYPES_H */

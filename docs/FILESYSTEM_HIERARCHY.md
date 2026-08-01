# VulcanOS Filesystem Hierarchy

This document specifies the **runtime** directory layout — what `/`
looks like on a booted VulcanOS system. This is a completely
different namespace from the *source repository's* layout described
in the top-level `README.md` (`kernel/`, `libc/`, `user/`, ...),
which is where VulcanOS's own code lives during development and
never appears on a running system's disk.

## Why not FHS

The traditional Unix `/bin` `/sbin` `/usr/bin` `/usr/local/bin` split
exists for a historical reason that no longer applies to any modern
system, VulcanOS included: early Unix's root partition was small and
`/usr` was often a second, larger disk mounted later in boot, so
"essential enough to survive without `/usr`" vs. "everything else"
was a meaningful distinction. VulcanOS has no such constraint from
day one — there's one disk, one filesystem, mounted once, at boot.
Preserving a four-way binary split that exists to solve a problem
VulcanOS doesn't have would be cargo-culting, not design. Likewise,
`/etc` and `/var` are abbreviations (*et cetera*, *variable*) whose
meaning isn't recoverable from the name itself — fine for something
learned once and never questioned, worse for the "clear,
developer-focused" identity this project is aiming for.

## Top-level layout

| Path | Purpose | Unix analog (for reference only — not copied) |
|---|---|---|
| `/vulcan` | All system binaries, unified — no bin/sbin/usr split | `/bin` + `/sbin` + `/usr/bin` |
| `/config` | System and package configuration | `/etc` |
| `/devices` | Device nodes (character/block special files) | `/dev` |
| `/home` | User home directories | `/home` (kept — already clear) |
| `/state` | Variable runtime data: logs, caches, spool, package database | `/var` |
| `/system` | Virtual, kernel-exposed filesystem: process list, hardware info, kernel stats — non-disk-backed | `/proc` + `/sys` |
| `/media` | Mount points for removable/external storage | `/mnt` + `/media` (unified) |
| `/packages` | Installed package payloads (the files a package actually ships) | closest to `/opt`, but the primary install location, not an optional extra |
| `/tmp` | Temporary files, cleared on boot | `/tmp` (kept — already clear) |

`/config` vs. `/packages` is a deliberate split worth calling out:
`/config` holds settings a person or `vulpkg` (the future package
manager) *edits*; `/packages` holds the payload a package *ships*
and that nothing but the package manager should touch directly. This
mirrors the same "settings vs. payload" distinction FHS gestures at
with `/etc` vs `/opt`, but stated as an explicit rule here rather
than left implicit.

## What this document does not yet cover

- Exact subdirectory structure within `/vulcan`, `/config`, etc. —
  designed as real filesystem code needs it, not speculatively ahead
  of time.
- The on-disk format for a persistent (non-RAM) filesystem — see
  `PROJECT_STATUS.md` for the current implementation, which is
  RAM-resident only (`vulcanfs`, an original tmpfs-style backend)
  until a real storage driver exists to persist to.
- Permissions model specifics beyond what's in the VFS header
  comments — a fuller access-control design is future work once
  there's real multi-user login to design it against.

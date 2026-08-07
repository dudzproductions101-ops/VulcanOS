# VulcanOS libc Architecture

VulcanOS includes a minimal libc that directly calls kernel functions due to the absence of ring‑3.
## 1. Direct Kernel Calls

Because there is no syscall boundary:

    malloc → kmalloc

    free → kfree

    open → vfs_open

    read → vfs_read

    write → vfs_write

    exit → scheduler termination

Function signatures match future userland APIs.
## 2. Implemented Components
String Functions

    strlen

    strcmp

    strcpy

    strcat

    strchr

    strdup

    memcpy

    memmove

    memset

    memcmp

Memory & Process

    malloc

    free

    atoi

    atol

    exit

I/O

    open

    read

    write

    close

    printf

    vulcan_readdir

## 3. FD Namespace Fix

Problem:

    VFS returns fds starting at 0

    libc reserves 0/1/2 for stdin/stdout/stderr

Fix:

    Introduced VULCAN_FD_OFFSET = 3

    libc translates fds before calling VFS

## 4. libc Self‑Test

Tests:
    
    String functions

    malloc/free

    atoi

    Full open/write/close/open/read/close round‑trip

    printf correctness

    Scheduler stability afterward

## 5. Future Work

    Real syscall boundary

    Userland isolation

    Buffered I/O

    File streams

    errno system

userland.md — Userland Architecture
VulcanOS Userland Architecture

Userland programs are currently compiled into the kernel and launched as threads.
## 1. Execution Model
Current Reality

    No ring‑3

    No exec()

    No ELF loader

    Programs are linked into kernel image

    Started as scheduler threads

## 2. init

Implemented in user/init/init.c.
Responsibilities

    PID 2

    Adapts void(void) → argc/argv

    Launches vulsh

## 3. Shell (vulsh)

Implemented in user/shell/vulsh.c.
Features

    Interactive line editing

    Backspace support

    Tokenization

    Built‑in command dispatch (direct function calls)

### Built‑ins

    help

    ls

    cat

    echo

## 4. Core Utilities
ls

    Uses vulcan_readdir

    Lists top‑level directories

### cat

    Uses libc open/read/close

    Reads arbitrary files

### echo

    Prints arguments

## 5. Interactive Verification

Verified via QEMU sendkey:

    help

    ls

    cat /state/fs-selftest.txt

Shell idle loop (poll + yield) is stable under soak test.
## 6. Future Work

    Real userland processes

    ELF loader

    exec()

    fork()

    Terminal emulator

    Desktop environment

    Networking utilities

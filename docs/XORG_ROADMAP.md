# VulcanOS Graphics & Xorg-Compatible Architecture

**Date:** 2026-09-01  
**Status:** Stage 1 (Graphics Syscalls) - **COMPLETE & COMPILING**

## Long-Term Vision

VulcanOS will evolve into a POSIX-compatible operating system capable of running X.Org X server and standard GUI applications.

```
VulcanOS Kernel (x86-64)
    ↓
    Process Management (ring-3 isolation)
    Address Spaces (VMM)
    Syscall Interface (stable ABI)
    Device Model (framebuffer, input, storage)
    ↓
POSIX-Compatible Userspace Libc
    ↓
X.Org X Server
    ↓
Window Manager / Desktop Environment (OpenBox, IceWM, XFCE, etc.)
    ↓
GUI Applications (Firefox, LibreOffice, etc.)
```

## Current Stage: Graphics Syscall Infrastructure

### What's Implemented ✓

**Kernel Side:**
- `syscall_handle()` dispatcher in kernel/core/syscall.c
- SYS_GRAPHICS_INFO (syscall 10) - Query framebuffer dimensions
- SYS_GRAPHICS_CLEAR (syscall 11) - Clear framebuffer to color
- SYS_GRAPHICS_DRAW_RECT (syscall 12) - Draw filled rectangles
- Validation and error handling for all graphics syscalls
- Graceful fallback to text mode if framebuffer unavailable

**Userland Side:**
- libc/graphics_syscall.s - x86-64 syscall wrappers (assembly)
- libc/include/vulcan_graphics.h - Userland graphics API
- user/graphics/graphics_server.c - Graphics service process
- Caching layer for framebuffer metadata queries

**ABI Documentation:**
- docs/SYSCALL_ABI.md - Complete syscall calling convention
- Frozen syscall numbers (will never change)
- Reserved ranges for future syscalls
- Stability guarantees for userland compatibility

### Architecture Decisions

1. **Syscall-Based Graphics** - No direct userland framebuffer access
   - Rationale: Xorg model, security, future output switching
   - Syscalls can be intercepted, validated, or redirected

2. **Register-Based Calling Convention** - Follow x86-64 ABI
   - RDI, RSI, RDX, R10, R8, R9 for arguments 1-6
   - RAX for syscall number and return value
   - Matches Linux/Unix conventions for easy POSIX implementation

3. **Graceful Degradation** - Text mode fallback
   - If framebuffer unavailable, graphics_server detects and exits
   - vulsh shell continues to work
   - No kernel panic on missing GPU

4. **Separate Process Model** - graphics_server as userland process
   - Not kernel code
   - Can be restarted, debugged, replaced
   - Foundation for true application isolation

### Compilation Status

```
✓ kernel/core/syscall.c       (syscall dispatcher, 120 lines)
✓ kernel/include/syscall.h    (syscall numbers and ABI)
✓ libc/graphics_syscall.s     (x86-64 assembly wrappers, 17 lines)
✓ libc/include/vulcan_graphics.h (userland graphics API)
✓ user/graphics/graphics_server.c (graphics service, 69 lines)
✓ kernel/build/vulcanos.elf   (303 KB, fully linked)
```

**Build Result:** ✅ SUCCESS - No linker errors, no undefined references

### Testing Next Steps

```bash
make iso         # Create bootable ISO
make run         # Boot in QEMU with graphics support
# Verify:
# 1. VulcanOS banner appears in text mode
# 2. Colored boot screen drawn (if framebuffer initialized)
# 3. vulsh shell prompt appears
# 4. No crashes or hangs
```

## Next Stages (In Order)

### Stage 2: Ring-3 Process Isolation (30-50 hours)

**Goal:** Real user processes with separate address spaces

**Milestones:**
- Create new ring-3 privilege level gate (user code segment)
- Implement process_exec(filename) to load ELF binaries
- Separate kernel address space from userland spaces
- Set up separate page tables per process
- Implement process_exit() properly (with cleanup)
- Syscall interface that respects privilege level

**Tests:**
- Load small user program from disk
- Run in isolated address space
- Prevent access to kernel memory
- Return to kernel on syscall/exception

### Stage 3: Process Management Syscalls (20-30 hours)

**New Syscalls:**
- SYS_EXEC - Load and execute program
- SYS_EXIT - Exit with status code
- SYS_WAIT - Wait for child process
- SYS_FORK - Clone current process
- SYS_KILL - Send signal to process

**Rationale:** Foundation for process trees, shells, backgrounding

### Stage 4: Virtual Memory Syscalls (30-40 hours)

**New Syscalls:**
- SYS_MMAP - Map memory region
- SYS_MUNMAP - Unmap memory region
- SYS_MREMAP - Resize memory region
- SYS_MPROTECT - Change protections
- SYS_BRK - Change heap size

**Rationale:** Let userland manage its own memory, malloc support

### Stage 5: VFS & File Descriptors (40-60 hours)

**New Syscalls:**
- SYS_OPEN - Open file
- SYS_CLOSE - Close file descriptor
- SYS_READ - Read from file/stdin
- SYS_WRITE - Write to file/stdout
- SYS_SEEK - Seek in file
- SYS_STAT - Get file metadata
- SYS_UNLINK - Delete file
- SYS_MKDIR - Create directory

**Rationale:** Necessary for C stdio (FILE streams), shell operations

### Stage 6: Device Abstraction (20-30 hours)

**Goal:** Abstract framebuffer, keyboard, mouse as /dev devices

**Changes:**
- /dev/fb0 - Framebuffer device
- /dev/input/event0 - Keyboard events
- /dev/input/mouse0 - Mouse events
- mmap() support for device memory

**Rationale:** Xorg will mmap /dev/fb0 to access framebuffer

### Stage 7: Input Handling (20-30 hours)

**Changes:**
- Queue keyboard events instead of direct keyboard_read()
- Mouse driver integration
- /dev/input event interface
- TTY/console line discipline

### Stage 8: IPC System (30-50 hours)

**New Syscalls:**
- SYS_PIPE - Create anonymous pipe
- SYS_SOCKET - Create Unix domain socket
- SYS_BIND - Bind socket to name
- SYS_LISTEN - Listen for connections
- SYS_ACCEPT - Accept connection
- SYS_CONNECT - Connect to socket
- SYS_SENDTO - Send data
- SYS_RECVFROM - Receive data

**Rationale:** Needed for X protocol (X server ↔ clients), shell job control

### Stage 9: POSIX Compliance (40-60 hours)

**Add:**
- Signal handling (SIGTERM, SIGKILL, SIGCHLD, etc.)
- Environment variables (setenv, getenv)
- Working directory (chdir, getcwd)
- User IDs (setuid, getuid - stub)
- File permissions (chmod, chown - stub)
- Process groups and sessions

**Rationale:** Required for standard Unix utilities and shells

### Stage 10: Xorg/Xserver Porting (100-200 hours)

**Options:**
1. Port XFree86 or X11 reference implementation
2. Build minimal Xvfb-like server
3. Compatibility layer on top of native graphics

**Prerequisites:** Stages 2-9 must be complete

**Integration Points:**
- mmap /dev/fb0 for framebuffer access
- /dev/input/event* for keyboard/mouse
- Unix sockets for X protocol clients
- Process forking for client isolation

### Stage 11: Window Manager & Desktop (50-100 hours)

**Build or Port:**
- OpenBox, IceWM, or minimal custom WM
- Runs on top of X server
- Manages window decorations, stacking, focus

### Stage 12: Applications (Ongoing)

- xterminal emulator
- File manager
- System monitor
- Text editor
- Eventually Firefox, LibreOffice, etc.

## ABI Stability Policy

**Once Stage 1 is frozen (NOW):**

- Syscall numbers will never change
- Struct layouts will never change (no reordering)
- Old syscall implementations kept forever (even if deprecated)
- New features = new syscall numbers only
- Kernel version queryable via syscall

**Example:** If we add SYS_GRAPHICS_FLUSH in the future, it becomes syscall 13. Syscall 11 (CLEAR) remains identical forever.

## Reference Documents

- **docs/SYSCALL_ABI.md** - Complete ABI specification (frozen as of now)
- **docs/ARCHITECTURE.md** - System architecture overview
- **docs/BOOT.md** - Boot process
- **docs/FILESYSTEM_HIERARCHY.md** - Directory structure
- **kernel/core/syscall.c** - Dispatcher implementation
- **libc/graphics_syscall.s** - Calling convention examples

## Known Limitations (Stage 1)

- ❌ No ring-3 user processes yet (all userland code runs in kernel)
- ❌ graphics_server and shell are hard-linked into kernel image
- ❌ No virtual memory isolation
- ❌ No file I/O syscalls
- ❌ No IPC (pipes, sockets)
- ❌ No signals
- ❌ No Xorg support yet

**These are intentional.** Each stage builds on previous foundations.

## Success Criteria

When all stages complete, VulcanOS will:

✅ Run real, isolated userspace processes  
✅ Support POSIX-compatible C library (glibc subset)  
✅ Run X.Org X server  
✅ Support standard GUI applications  
✅ Be recognizable as "Unix-like" to developers  

## Related Reading

- [Linux x86-64 ABI](https://en.wikipedia.org/wiki/X86_calling_conventions)
- [Linux Syscall Interface](https://linux.die.net/man/2/intro)
- [X.Org Architecture](https://www.x.org/wiki/)
- [OSDev Tutorials](https://wiki.osdev.org/)


# VulcanOS Syscall ABI v1.0

**Frozen as of:** 2026-09-01  
**Architecture:** x86-64  
**Toolchain:** GCC 9+ (freestanding)

## Calling Convention

All syscalls use the x86-64 calling convention with the following modifications:

### Register Allocation (Input)
```
RAX  = syscall number
RDI  = argument 1 (arg1)
RSI  = argument 2 (arg2)
RDX  = argument 3 (arg3)
R10  = argument 4 (arg4)   [NOT RCX! syscall clobbers rcx]
R8   = argument 5 (arg5)
R9   = argument 6 (arg6)
```

### Register Allocation (Output)
```
RAX  = return value (u64)
```

### Clobbered Registers
```
RCX  = clobbered by syscall instruction
R11  = clobbered by syscall instruction
```

## Syscall Numbers (Current v1)

### Core Process Management
| Number | Name | Status | Notes |
|--------|------|--------|-------|
| 1 | SYS_GETPID | STUB | Returns 1 (stub) |
| 2 | SYS_READ | STUB | Reads keyboard input |
| 3 | SYS_WRITE | STUB | Not yet implemented |
| 4 | SYS_EXIT | IMPLEMENTED | Halts current process |

### Graphics Subsystem (10-19)
| Number | Name | Status | Prototype |
|--------|------|--------|-----------|
| 10 | SYS_GRAPHICS_INFO | IMPLEMENTED | `u32 graphics_info(struct graphics_info *info)` |
| 11 | SYS_GRAPHICS_CLEAR | IMPLEMENTED | `void graphics_clear(u32 color)` |
| 12 | SYS_GRAPHICS_DRAW_RECT | IMPLEMENTED | `void graphics_draw_rect(u32 x, u32 y, u32 width, u32 height, u32 color)` |

### Reserved Ranges
- 5-9: Reserved for future core syscalls
- 13-19: Reserved for graphics extensions (sprite drawing, text, etc.)
- 20-49: Reserved for VFS/file descriptor syscalls
- 50-99: Reserved for memory management syscalls (mmap, munmap, etc.)
- 100-127: Reserved for IPC syscalls (pipes, sockets, etc.)
- 128+: Reserved for future extensions

## Graphics Subsystem Design

### Rationale
The graphics subsystem is designed for eventual Xorg/Xserver compatibility:
- Userland cannot directly access framebuffer memory
- All graphics operations go through kernel syscalls
- Kernel validates parameters before rendering
- Allows future: output switching, mode changes, hotplug events

### SYS_GRAPHICS_INFO (syscall 10)

**Prototype:**
```c
u32 graphics_info(struct graphics_info *info);
```

**Input:**
- RDI = pointer to `struct graphics_info` (in userland memory)

**Output:**
- RAX = 0 on success, 1 on failure (framebuffer unavailable)
- *info = filled with framebuffer dimensions

**Struct Definition:**
```c
struct graphics_info {
    u32 width;        // Framebuffer width in pixels
    u32 height;       // Framebuffer height in pixels
    u32 bpp;          // Bits per pixel (32-bit ARGB assumed)
    u8  available;    // 1 if framebuffer initialized, 0 otherwise
};
```

**Errors:**
- Return 1 if framebuffer not initialized or info pointer is NULL
- If unavailable, *info unchanged

**Note:** This syscall does not copy framebuffer memory to userland. It only returns metadata.

### SYS_GRAPHICS_CLEAR (syscall 11)

**Prototype:**
```c
void graphics_clear(u32 color);
```

**Input:**
- RDI = color in ARGB format (0xAARRGGBB)
  - For 32-bit framebuffer: all pixels set to this value
  - High byte (alpha) currently unused but reserved

**Output:**
- RAX = undefined (no return value)

**Side Effects:**
- Entire framebuffer filled with color
- No bounds checking (syscall is assumed safe)

**Future:** Alpha blending, pattern fills

### SYS_GRAPHICS_DRAW_RECT (syscall 12)

**Prototype:**
```c
void graphics_draw_rect(u32 x, u32 y, u32 width, u32 height, u32 color);
```

**Input:**
- RDI = x coordinate (top-left, pixels from left edge)
- RSI = y coordinate (top-left, pixels from top edge)
- RDX = width in pixels
- R10 = height in pixels
- R8  = color in ARGB format (0xAARRGGBB)

**Output:**
- RAX = undefined (no return value)

**Side Effects:**
- Fills rectangle [x, x+width) x [y, y+height) with color
- Clips to framebuffer bounds (no out-of-bounds writes)

**Future:** Rotation, alpha blending, pattern fills, ROP codes

## Future Extensions (Not Yet Implemented)

### Graphics (13-19)
- SYS_GRAPHICS_DRAW_TEXT - Rasterize glyphs at position
- SYS_GRAPHICS_COPY_RECT - Copy one region to another (blitting)
- SYS_GRAPHICS_FILL_PATTERN - Fill with pattern/tile
- SYS_GRAPHICS_FLIP_BUFFER - For double-buffering
- SYS_GRAPHICS_GET_MODE - Query current video mode
- SYS_GRAPHICS_SET_MODE - Change resolution

### Input Events (planned)
- Keyboard events via syscall or /dev/input style
- Mouse position + button state
- Event queue or polling

### Window Management (planned, post-Xorg-compat)
- Will be handled by X.Org X server in userland
- Kernel provides only basic framebuffer access
- WM handles window stacking, focus, etc.

## Stability Notes

### ABI Stability Guarantee
- Syscall numbers **WILL NOT CHANGE** once frozen
- Struct layouts **WILL NOT CHANGE** (no field reordering)
- New features added via new syscall numbers, never overloading old ones
- Parameter passing convention **WILL NOT CHANGE**

### Implementation Compatibility
- Kernel may add new syscalls at higher numbers
- Userland must gracefully handle unknown syscall numbers (return -1)
- Userland should check graphics_info.available before drawing

### Version Checking
Future: Add SYS_SYSCALL_VERSION to query kernel ABI version.

## Implementation Files

| File | Role |
|------|------|
| kernel/core/syscall.c | Syscall dispatcher (all numbers) |
| kernel/include/syscall.h | Syscall number constants |
| libc/graphics_syscall.s | Userland syscall wrappers (assembly) |
| libc/include/vulcan_graphics.h | Userland graphics API |
| user/graphics/graphics_server.c | Userland graphics service process |

## Testing Checklist

- [x] Compilation without errors
- [ ] Boot VulcanOS and verify no hang
- [ ] graphics_info returns valid framebuffer dimensions
- [ ] graphics_clear fills framebuffer with color
- [ ] graphics_draw_rect draws colored rectangles
- [ ] vulsh text mode works if framebuffer unavailable
- [ ] Multiple graphics_server invocations don't crash

## Roadmap

After graphics syscalls stable, implement (in order):

1. **Ring-3 Process Isolation** - Real userspace processes with separate address spaces
2. **Process Syscalls** - exec, exit, wait, fork
3. **Virtual Memory** - mmap/munmap syscalls, proper page tables per process
4. **VFS Expansion** - File descriptors, open/read/write/close syscalls
5. **Device Abstraction** - /dev filesystem for device access
6. **Input Devices** - Keyboard/mouse via /dev/input or syscalls
7. **IPC Primitives** - Pipes, shared memory, Unix sockets
8. **POSIX Compatibility** - Signal handling, environment variables, etc.
9. **Xorg/Xserver Port** - Adapt XFree86 or minimal Xvfb clone
10. **Window Manager** - Build on Xorg foundation
11. **Desktop Environment** - GUI shell, applications


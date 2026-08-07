#include "kernel.h"
#include "printk.h"
#include "panic.h"
#include "config.h"
#include "multiboot2.h"
#include "drivers/console.h"
#include "drivers/driver.h"
#include "drivers/framebuffer.h"
#include "drivers/timer.h"
#include "drivers/keyboard.h"
#include "drivers/display.h"
#include "drivers/block.h"
#include "drivers/pci.h"
#include "drivers/ahci.h"
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/interrupts.h"
#include "arch/x86_64/cpu.h"
#include "mm/pmm.h"
#include "mm/paging.h"
#include "mm/allocator.h"
#include "proc/process.h"
#include "proc/thread.h"
#include "proc/scheduler.h"
#include "fs/vfs.h"
#include "fs/vulcanfs.h"
#include "drivers/block_dev.h"
#include "graphics.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "init.h"
#include "pkg/vpkg.h"
#include "pkg/embedded_packages.h"

static void print_banner(void)
{
    console_set_color(VGA_LIGHT_RED, VGA_BLACK);
    printk("%s %d.%d.%d \"%s\"\n",
           VULCAN_OS_NAME, VULCAN_VERSION_MAJ, VULCAN_VERSION_MIN,
           VULCAN_VERSION_PATCH, VULCAN_CODENAME);
    console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    printk("a Unix-like operating system, built from scratch.\n\n");
}

/* Provided by linker.ld; the kernel's own physical footprint, used
 * by paging_init to build the higher-half mapping and by pmm_init
 * (via pmm.c's own extern) to reserve those frames. */
extern u8 kernel_start[];
extern u8 kernel_end[];

/* A crude busy-wait, NOT a real sleep primitive -- VulcanOS has no
 * timer-based blocking/wake mechanism yet (that needs the scheduler
 * to support THREAD_BLOCKED with a wake condition, which is real
 * future work, not implemented by this bring-up milestone). This
 * exists only so the two demo threads below print at a human-
 * readable pace instead of flooding the console faster than a
 * screenshot could ever capture, so the round-robin rotation is
 * actually visible rather than just theoretically true. Busy-
 * waiting like this wastes the entire time slice spinning -- a real
 * blocking sleep would instead call scheduler_reschedule() and let
 * other threads run during the wait, which is exactly what a future
 * sleep() implementation should do instead of this. */
static void crude_busy_wait(u64 iterations)
{
    for (volatile u64 i = 0; i < iterations; i++) {
        __asm__ volatile ("nop");
    }
}

/* Demo thread A: proves round-robin actually rotates between two
 * independently-created threads, not just "the scheduler runs one
 * thread forever." Deliberately simple -- printing a counter is
 * enough to make interleaving visible on screen; it is not meant to
 * demonstrate anything beyond "two threads genuinely alternate." */
/* __attribute__((unused)): kept as real, working, documented
 * scheduler-verification code (see PROJECT_STATUS.md's "Scheduler
 * bring-up" section for what these two functions proved and how)
 * even though init_thread_entry/vulsh are what actually boots by
 * default now -- see kmain's own comment at its process_create
 * calls. Without this attribute, GCC's -Wunused-function would flag
 * these as dead code, which they are not: they're intentionally-
 * not-currently-enqueued verification code, not a mistake. */
__attribute__((unused)) static void demo_thread_a(void)
{
    for (u32 i = 0; i < 5; i++) {
        printk_level(LOG_INFO, "demo-thread-a: iteration %u\n", i);
        crude_busy_wait(30000000);
    }
    printk_level(LOG_INFO, "demo-thread-a: finished, exiting\n");
}

__attribute__((unused)) static void demo_thread_b(void)
{
    for (u32 i = 0; i < 5; i++) {
        printk_level(LOG_INFO, "demo-thread-b: iteration %u\n", i);
        crude_busy_wait(30000000);
    }
    printk_level(LOG_INFO, "demo-thread-b: finished, exiting\n");
}

/* The idle thread: what runs when nothing else is ready. Every
 * scheduler needs one -- switch_to_next (scheduler.c) would panic
 * with "system deadlocked" the moment every other thread finishes
 * or blocks if there were nothing left to hand the CPU to. */
static void idle_thread_entry(void)
{
    for (;;) {
        hlt();
    }
}

/* Brings up the filesystem: initializes the VFS, creates a vulcanfs
 * instance, mounts it at "/", and builds VulcanOS's real top-level
 * hierarchy (see docs/FILESYSTEM_HIERARCHY.md for the design
 * rationale behind each name). Kept as its own function rather than
 * inlined into kmain, matching the style of the other bring-up
 * steps -- each subsystem's initialization is one clearly-named
 * call, not a wall of inline logic. */
static usize append_u32_decimal(char *buf, usize offset, u32 value)
{
    char temp[16];
    int len = 0;

    if (value == 0) {
        temp[len++] = '0';
    } else {
        while (value > 0) {
            temp[len++] = '0' + (value % 10);
            value /= 10;
        }
    }

    for (int i = len - 1; i >= 0; i--) {
        buf[offset++] = temp[i];
    }
    return offset;
}

static void fs_bringup(void)
{
    vfs_init();

    struct inode *root = vulcanfs_create();
    if (!root) {
        panic("fs_bringup: failed to create vulcanfs root");
    }
    if (!vfs_mount("/", root)) {
        panic("fs_bringup: failed to mount vulcanfs at /");
    }

    static const char *toplevel_dirs[] = {
        "/vulcan", "/config", "/devices", "/home",
        "/state", "/system", "/media", "/packages", "/tmp",
    };
    for (usize i = 0; i < sizeof(toplevel_dirs) / sizeof(toplevel_dirs[0]); i++) {
        if (!vfs_create(toplevel_dirs[i], INODE_DIRECTORY)) {
            printk_level(LOG_WARN, "fs_bringup: failed to create %s\n", toplevel_dirs[i]);
        }
    }

    if (framebuffer_available()) {
        struct inode *devices_dir = vfs_resolve("/devices");
        if (devices_dir && devices_dir->type == INODE_DIRECTORY) {
            if (!vulcanfs_create_device(devices_dir, "fb0", VULCANFS_DEVICE_FRAMEBUFFER)) {
                printk_level(LOG_WARN, "fs_bringup: failed to create framebuffer device node\n");
            } else {
                printk_level(LOG_INFO, "fs_bringup: framebuffer device node /devices/fb0 created\n");
            }
        }

        const struct framebuffer_info *fb = framebuffer_info();
        if (fb && fb->available) {
            char info_text[128];
            usize len = 0;

            const char *prefix = "framebuffer: width=";
            for (const char *p = prefix; *p; p++) {
                info_text[len++] = *p;
            }
            len = append_u32_decimal(info_text, len, fb->width);
            const char *mid1 = ", height=";
            for (const char *p = mid1; *p; p++) {
                info_text[len++] = *p;
            }
            len = append_u32_decimal(info_text, len, fb->height);
            const char *mid2 = ", bpp=";
            for (const char *p = mid2; *p; p++) {
                info_text[len++] = *p;
            }
            len = append_u32_decimal(info_text, len, fb->bpp);
            const char *mid3 = ", pitch=";
            for (const char *p = mid3; *p; p++) {
                info_text[len++] = *p;
            }
            len = append_u32_decimal(info_text, len, fb->pitch);
            info_text[len++] = '\n';

            if (vfs_create("/system/display.info", INODE_FILE)) {
                int fd = open("/system/display.info", VULCAN_O_WRITE | VULCAN_O_CREATE);
                if (fd >= 0) {
                    write(fd, info_text, len);
                    close(fd);
                }
            }

            /* Write PCI device report if available */
            const char *pci_report = pci_get_report();
            usize pci_len = pci_get_report_len();
            if (pci_report && pci_len > 0) {
                if (vfs_create("/system/pci.devices", INODE_FILE)) {
                    int pfd = open("/system/pci.devices", VULCAN_O_WRITE | VULCAN_O_CREATE);
                    if (pfd >= 0) {
                        write(pfd, pci_report, pci_len);
                        close(pfd);
                    }
                }
            }
        }
    }

    /* Create device nodes for all block devices registered before
     * filesystem bring-up (ramdisk, AHCI, etc.). */
    struct inode *devices_dir = vfs_resolve("/devices");
    if (devices_dir && devices_dir->type == INODE_DIRECTORY) {
        usize device_count = block_device_count();
        for (usize i = 0; i < device_count; i++) {
            const char *name = block_device_name(i);
            if (!name) {
                continue;
            }
            if (!vulcanfs_create_device(devices_dir, name, VULCANFS_DEVICE_BLOCK)) {
                printk_level(LOG_WARN, "fs_bringup: failed to create block device node /devices/%s\n", name);
            } else {
                printk_level(LOG_INFO, "fs_bringup: block device node /devices/%s created\n", name);
            }
        }
    }

    /* Verification, not just "no crash": create a real file, write
     * real content, close it, reopen it fresh, read it back, and
     * compare byte-for-byte. This is the same standard every other
     * subsystem in this bring-up was held to (mm's pmm/paging
     * numbers were checked against real Multiboot2 data; the
     * scheduler's rotation was checked against real interleaved
     * output) -- filesystem correctness gets the same discipline. */
    const char *test_path = "/state/fs-selftest.txt";
    const char *test_content = "VulcanOS filesystem self-test: read-after-write verified.";
    usize content_len = 0;
    while (test_content[content_len]) {
        content_len++;
    }

    if (!vfs_create(test_path, INODE_FILE)) {
        panic("fs_bringup: self-test failed to create test file");
    }

    vulcan_fd_t write_fd = vfs_open(test_path);
    if (write_fd == VULCAN_FD_INVALID) {
        panic("fs_bringup: self-test failed to open test file for writing");
    }
    isize written = vfs_write(write_fd, test_content, content_len);
    vfs_close(write_fd);

    if (written != (isize)content_len) {
        panic("fs_bringup: self-test write returned unexpected byte count");
    }

    vulcan_fd_t read_fd = vfs_open(test_path);
    if (read_fd == VULCAN_FD_INVALID) {
        panic("fs_bringup: self-test failed to reopen test file for reading");
    }
    char readback[128];
    isize read_count = vfs_read(read_fd, readback, sizeof(readback) - 1);
    vfs_close(read_fd);

    if (read_count != (isize)content_len) {
        panic("fs_bringup: self-test read returned unexpected byte count");
    }
    readback[read_count] = '\0';

    for (usize i = 0; i < content_len; i++) {
        if (readback[i] != test_content[i]) {
            panic("fs_bringup: self-test read-back content mismatch");
        }
    }

    printk_level(LOG_INFO, "fs: self-test passed (%llu-byte write, read-back verified byte-for-byte)\n",
                 (u64)content_len);
}

/* Verifies libc's own functions work correctly, end to end -- not
 * just that they link. Deliberately exercises libc's PUBLIC API
 * (string.h/stdlib.h/stdio.h's open/read/write, not vfs_* directly)
 * so this test actually proves libc's wrapper layer is correct, on
 * top of what fs_bringup already proved about the VFS itself. Must
 * run AFTER fs_bringup, since it writes to /state, which fs_bringup
 * is what actually creates. Same standard every prior subsystem was
 * held to: real assertions against real computed values, panicking
 * on any mismatch, not "ran without crashing." */
static void libc_selftest(void)
{
    /* string.c: strlen, strcmp, strcpy, memmove overlap */
    if (strlen("VulcanOS") != 8) {
        panic("libc_selftest: strlen mismatch");
    }
    if (strcmp("abc", "abc") != 0) {
        panic("libc_selftest: strcmp equal-strings mismatch");
    }
    if (strcmp("abc", "abd") >= 0) {
        panic("libc_selftest: strcmp ordering mismatch");
    }

    char buf1[16];
    strcpy(buf1, "hello");
    if (strcmp(buf1, "hello") != 0) {
        panic("libc_selftest: strcpy round-trip mismatch");
    }

    char overlap[10] = {0,1,2,3,4,5,6,7,8,9};
    memmove(overlap + 4, overlap + 2, 5); /* dest > src, overlapping */
    static const char expected_overlap[5] = {2,3,4,5,6};
    for (int i = 0; i < 5; i++) {
        if (overlap[4 + i] != expected_overlap[i]) {
            panic("libc_selftest: memmove overlap handling incorrect");
        }
    }

    /* stdlib.c: malloc/free, atoi */
    int *heap_int = malloc(sizeof(int));
    if (!heap_int) {
        panic("libc_selftest: malloc returned NULL");
    }
    *heap_int = 42;
    if (*heap_int != 42) {
        panic("libc_selftest: malloc'd memory didn't hold its value");
    }
    free(heap_int);

    if (atoi("1234") != 1234) {
        panic("libc_selftest: atoi mismatch");
    }
    if (atoi("-56") != -56) {
        panic("libc_selftest: atoi negative-number mismatch");
    }

    /* stdio.c: open/write/close/open/read/close through libc's OWN
     * wrapper functions (not vfs_* directly), plus printf itself. */
    const char *path = "/state/libc-selftest.txt";
    const char *content = "libc self-test content";
    usize content_len = strlen(content);

    int wfd = open(path, VULCAN_O_WRITE | VULCAN_O_CREATE);
    if (wfd < 0) {
        panic("libc_selftest: open-for-write failed");
    }
    ssize_t written = write(wfd, content, content_len);
    close(wfd);
    if (written != (ssize_t)content_len) {
        panic("libc_selftest: write returned unexpected byte count");
    }

    int rfd = open(path, VULCAN_O_READ);
    if (rfd < 0) {
        panic("libc_selftest: open-for-read failed");
    }
    char readback[64];
    ssize_t read_count = read(rfd, readback, sizeof(readback) - 1);
    close(rfd);
    if (read_count != (ssize_t)content_len) {
        panic("libc_selftest: read returned unexpected byte count");
    }
    readback[read_count] = '\0';
    if (strcmp(readback, content) != 0) {
        panic("libc_selftest: read-back content mismatch");
    }

    printk_level(LOG_INFO, "libc: self-test passed (string/stdlib/stdio all verified)\n");
    printf("libc printf() is alive: %d + %d = %d, string=\"%s\"\n", 2, 2, 4, "vulcan");
}

/* Verifies vpkg end to end: installs the real embedded hello-vulcan
 * package (built by tools/vpkbuild.py from tools/packages/
 * hello-vulcan/, embedded via tools/bin2c.py -- see
 * pkg/embedded_packages.h for the full workflow), then checks EVERY
 * layer actually worked: the database record exists with the right
 * fields, AND the actual installed files exist at their real
 * destinations with the exact content the source package shipped.
 * Checking only the database record would leave real installation
 * bugs (e.g. a manifest parsed correctly but a file copy silently
 * failing) undetected -- this test deliberately checks both. */
static void vpkg_selftest(void)
{
    const struct embedded_package *pkg = embedded_package_find("hello-vulcan");
    if (!pkg) {
        panic("vpkg_selftest: hello-vulcan is not registered in embedded_packages.c");
    }

    enum vpkg_result result = vpkg_install(pkg->data, pkg->size);
    if (result != VPKG_OK) {
        panic("vpkg_selftest: vpkg_install(hello-vulcan) failed");
    }

    if (vpkg_installed_count() != 1) {
        panic("vpkg_selftest: expected exactly 1 installed package after install");
    }

    struct vpkg_record rec;
    if (!vpkg_list(0, &rec)) {
        panic("vpkg_selftest: vpkg_list(0) failed after a successful install");
    }
    if (strcmp(rec.name, "hello-vulcan") != 0) {
        panic("vpkg_selftest: installed record has wrong name");
    }
    if (strcmp(rec.version, "1.0.0") != 0) {
        panic("vpkg_selftest: installed record has wrong version");
    }
    if (rec.file_count != 2) {
        panic("vpkg_selftest: installed record has wrong file_count");
    }

    /* Verify the ACTUAL installed file, not just the database
     * record -- open it fresh through libc's own open()/read(),
     * exactly like a real user's `cat` would, and check its content
     * matches what tools/packages/hello-vulcan/config/hello.conf
     * actually contains. */
    int fd = open("/config/hello-vulcan.conf", VULCAN_O_READ);
    if (fd < 0) {
        panic("vpkg_selftest: installed file /config/hello-vulcan.conf not found");
    }
    char buf[256];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) {
        panic("vpkg_selftest: failed to read installed config file");
    }
    buf[n] = '\0';
    if (!strstr(buf, "greeting = Hello, VulcanOS!")) {
        panic("vpkg_selftest: installed config file has unexpected content");
    }

    printk_level(LOG_INFO,
                 "vpkg: self-test passed (hello-vulcan installed, database record and "
                 "actual file content both verified)\n");
}

void kmain(u64 mb2_magic, u64 mb2_info_addr)
{
    console_init();
    print_banner();

    printk_level(LOG_INFO, "console initialized (VGA text mode, 80x25)\n");

    if (mb2_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        panic("kmain: invalid Multiboot2 magic; not booted by an Multiboot2-compliant loader");
    }

    framebuffer_init(mb2_info_addr);
    framebuffer_driver_register();
    /* register storage early so fs_bringup can see block devices */
    ramdisk_driver_register();
    driver_init_all();
    graphics_init();
    if (display_available()) {
        printk_level(LOG_INFO, "graphics: display initialized and boot UI rendered\n");
    } else {
        printk_level(LOG_INFO, "graphics: no display available, falling back to VGA text mode\n");
    }

    gdt_install();
    printk_level(LOG_INFO, "GDT installed (kernel/user code+data, TSS)\n");

    interrupts_install();
    printk_level(LOG_INFO, "IDT installed, PIC remapped to vectors 32-47\n");

    cpu_print_info();

    /* mm bring-up, strictly in dependency order: pmm hands out raw
     * physical frames; paging needs pmm to allocate page-table
     * levels and then takes over from boot.asm's temporary identity
     * map; kheap needs paging to back its virtual address range
     * with real physical memory as it grows. */
    pmm_init(mb2_info_addr);
    paging_init((paddr_t)(uptr)kernel_start, (paddr_t)(uptr)kernel_end);
    kheap_init();

    timer_init();
    printk_level(LOG_INFO, "PIT timer initialized (%d Hz)\n", PIT_FREQUENCY_HZ);

    keyboard_init();
    printk_level(LOG_INFO, "PS/2 keyboard driver initialized\n");

    /* Register important PCI drivers that should bind during PCI scan */
    ahci_driver_register();

    /* Discover PCI devices early so drivers can be bound; the
     * textual report will be written into the filesystem by
     * fs_bringup() after the VFS is mounted. */
    pci_init();

    fs_bringup();
    vpkg_init();
    libc_selftest();
    vpkg_selftest();

    scheduler_init();

    /* init_thread_entry (user/init/init.c) is VulcanOS's real
     * userland entry point: it starts vulsh, which is where an
     * interactive user actually lands. demo_thread_a/demo_thread_b
     * (defined earlier in this file) remain available, tested,
     * documented scheduler-verification code -- see
     * PROJECT_STATUS.md's scheduler bring-up writeup for what they
     * proved and how -- but are no longer enqueued by default, since
     * their job (proving round-robin preemption genuinely works,
     * not just appears to) is done and their proof is preserved in
     * that writeup rather than needing to re-run on every boot. */
    struct process *idle_proc = process_create("idle", idle_thread_entry);
    struct process *init_proc = process_create("init", init_thread_entry);

    scheduler_enqueue(idle_proc->threads[0]);
    scheduler_enqueue(init_proc->threads[0]);

    printk("\n");
    printk_level(LOG_INFO, "bring-up complete. starting scheduler; init (pid=%llu) will start vulsh.\n\n",
                 init_proc->pid);

    /* Never returns: from this point on, "the kernel's execution"
     * IS whichever thread the scheduler has chosen, forever. The
     * function that got us here (kmain) has no more meaning as a
     * call stack VulcanOS will ever unwind back through. */
    scheduler_start();
}

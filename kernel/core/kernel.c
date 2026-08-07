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

extern u8 kernel_start[];
extern u8 kernel_end[];

static void crude_busy_wait(u64 iterations)
{
    for (volatile u64 i = 0; i < iterations; i++) {
        __asm__ volatile ("nop");
    }
}

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

static void idle_thread_entry(void)
{
    for (;;) {
        hlt();
    }
}

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

static void libc_selftest(void)
{

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
    memmove(overlap + 4, overlap + 2, 5); 
    static const char expected_overlap[5] = {2,3,4,5,6};
    for (int i = 0; i < 5; i++) {
        if (overlap[4 + i] != expected_overlap[i]) {
            panic("libc_selftest: memmove overlap handling incorrect");
        }
    }

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

    pmm_init(mb2_info_addr);
    paging_init((paddr_t)(uptr)kernel_start, (paddr_t)(uptr)kernel_end);
    kheap_init();

    timer_init();
    printk_level(LOG_INFO, "PIT timer initialized (%d Hz)\n", PIT_FREQUENCY_HZ);

    keyboard_init();
    printk_level(LOG_INFO, "PS/2 keyboard driver initialized\n");

    ahci_driver_register();

    pci_init();

    fs_bringup();
    vpkg_init();
    libc_selftest();
    vpkg_selftest();

    scheduler_init();

    struct process *idle_proc = process_create("idle", idle_thread_entry);
    struct process *init_proc = process_create("init", init_thread_entry);

    scheduler_enqueue(idle_proc->threads[0]);
    scheduler_enqueue(init_proc->threads[0]);

    printk("\n");
    printk_level(LOG_INFO, "bring-up complete. starting scheduler; init (pid=%llu) will start vulsh.\n\n",
                 init_proc->pid);

    scheduler_start();
}

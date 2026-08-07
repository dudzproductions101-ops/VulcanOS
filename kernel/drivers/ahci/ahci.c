#include "drivers/ahci.h"
#include "drivers/pci.h"
#include "drivers/block_dev.h"
#include "printk.h"
#include "mm/paging.h"
#include "mm/allocator.h"
#include "config.h"
#include "string.h"

#define AHCI_MAX_PORTS 32
#define AHCI_CMD_LIST_SIZE 1024
#define AHCI_FIS_SIZE 256
#define AHCI_CMD_TABLE_SIZE 4096
#define AHCI_CMD_TABLE_ALIGN 128
#define AHCI_SECTOR_SIZE 512
#define AHCI_PRDT_BYTE_COUNT_MAX ((1u << 22) - 1)

#define AHCI_PORT_CMD_ST  (1u << 0)
#define AHCI_PORT_CMD_FRE (1u << 4)
#define AHCI_PORT_CMD_FR  (1u << 14)
#define AHCI_PORT_CMD_CR  (1u << 15)

#define AHCI_FIS_TYPE_REG_H2D 0x27
#define AHCI_CMD_READ 0x25
#define AHCI_CMD_WRITE 0x35
#define AHCI_CMD_IDENTIFY 0xEC

struct ahci_port {
    u32 clb;
    u32 clbu;
    u32 fb;
    u32 fbu;
    u32 is;
    u32 ie;
    u32 cmd;
    u32 rsv0;
    u32 tfd;
    u32 sig;
    u32 ssts;
    u32 sctl;
    u32 serr;
    u32 sact;
    u32 ci;
    u32 sntf;
    u32 fbs;
    u32 rsv1[11];
    u32 vendor[4];
};

struct ahci_hba_mem {
    u32 cap;
    u32 ghc;
    u32 is;
    u32 pi;
    u32 vs;
    u32 ccc_ctl;
    u32 ccc_pts;
    u32 em_loc;
    u32 cap2;
    u32 bohc;
    u8  reserved[0xA0 - 0x28];
    u8  vendor[0x60];
    struct ahci_port ports[AHCI_MAX_PORTS];
};

struct ahci_cmd_header {
    u32 dw0;
    u32 prdbc;
    u32 ctba;
    u32 ctbau;
    u32 reserved[4];
};

struct ahci_prdt_entry {
    u32 dba;
    u32 dbau;
    u32 reserved;
    u32 dbc;
};

struct ahci_cmd_table {
    u8 cfis[64];
    u8 acmd[16];
    u8 reserved[48];
    struct ahci_prdt_entry prdt[1];
};

struct ahci_port_state {
    volatile struct ahci_port *port;
    void *cmd_list_virt;
    void *fis_base_virt;
    void *cmd_table_virt;
    paddr_t cmd_table_phys;
    char name[8];
    u64 sector_count;
};

static struct ahci_port_state ahci_states[AHCI_MAX_PORTS];

static void *align_ptr(void *ptr, usize align)
{
    uptr value = (uptr)ptr;
    uptr aligned = (value + align - 1) & ~(align - 1);
    return (void *)aligned;
}

static paddr_t virt_to_phys(void *virt)
{
    return paging_virt_to_phys((vaddr_t)(uptr)virt);
}

static bool ahci_port_device_present(volatile struct ahci_port *port)
{
    u32 ssts = port->ssts;
    u8 det = ssts & 0x0F;
    u8 ipm = (ssts >> 8) & 0x0F;
    return det == 3 && ipm == 1;
}

static void ahci_port_start(volatile struct ahci_port *port)
{
    while (port->cmd & AHCI_PORT_CMD_CR) {

    }
    port->cmd |= AHCI_PORT_CMD_FRE;
    port->cmd |= AHCI_PORT_CMD_ST;
}

static bool ahci_port_configure(struct ahci_port_state *state)
{
    volatile struct ahci_port *port = state->port;

    void *raw_cmd_list = kmalloc(AHCI_CMD_LIST_SIZE + AHCI_CMD_LIST_SIZE);
    if (!raw_cmd_list) {
        return false;
    }
    void *cmd_list = align_ptr(raw_cmd_list, AHCI_CMD_LIST_SIZE);

    void *raw_fis = kmalloc(AHCI_FIS_SIZE + AHCI_FIS_SIZE);
    if (!raw_fis) {
        return false;
    }
    void *fis_base = align_ptr(raw_fis, AHCI_FIS_SIZE);

    void *raw_cmd_table = kmalloc(AHCI_CMD_TABLE_SIZE * AHCI_MAX_PORTS + AHCI_CMD_TABLE_ALIGN);
    if (!raw_cmd_table) {
        return false;
    }
    void *cmd_table = align_ptr(raw_cmd_table, AHCI_CMD_TABLE_ALIGN);

    memset(cmd_list, 0, AHCI_CMD_LIST_SIZE);
    memset(fis_base, 0, AHCI_FIS_SIZE);
    memset(cmd_table, 0, AHCI_CMD_TABLE_SIZE * AHCI_MAX_PORTS);

    paddr_t cmd_list_phys = virt_to_phys(cmd_list);
    paddr_t fis_base_phys = virt_to_phys(fis_base);
    paddr_t cmd_table_phys = virt_to_phys(cmd_table);
    if (cmd_list_phys == 0 || fis_base_phys == 0 || cmd_table_phys == 0) {
        return false;
    }

    port->clb = (u32)(cmd_list_phys & 0xFFFFFFFFULL);
    port->clbu = (u32)(cmd_list_phys >> 32);
    port->fb = (u32)(fis_base_phys & 0xFFFFFFFFULL);
    port->fbu = (u32)(fis_base_phys >> 32);

    state->cmd_list_virt = cmd_list;
    state->fis_base_virt = fis_base;
    state->cmd_table_virt = cmd_table;
    state->cmd_table_phys = cmd_table_phys;

    ahci_port_start(port);
    return true;
}

static bool ahci_port_find_free_slot(volatile struct ahci_port *port, u32 *slot_out)
{
    u32 occupied = port->sact | port->ci;
    for (u32 idx = 0; idx < AHCI_MAX_PORTS; idx++) {
        if (!(occupied & (1u << idx))) {
            *slot_out = idx;
            return true;
        }
    }
    return false;
}

static bool ahci_port_issue_command(struct ahci_port_state *state, bool write,
                                   u64 lba, u32 sector_count, void *buf,
                                   usize buf_bytes, u8 command)
{
    if (!state || !state->port || !buf || buf_bytes == 0) {
        return false;
    }

    volatile struct ahci_port *port = state->port;
    u32 slot;
    if (!ahci_port_find_free_slot(port, &slot)) {
        return false;
    }

    volatile struct ahci_cmd_header *cmd_list = (volatile struct ahci_cmd_header *)state->cmd_list_virt;
    volatile struct ahci_cmd_header *cmd_header = &cmd_list[slot];
    volatile struct ahci_cmd_table *cmd_table = (volatile struct ahci_cmd_table *)((u8 *)state->cmd_table_virt + slot * AHCI_CMD_TABLE_SIZE);

    memset((void *)cmd_header, 0, sizeof(*cmd_header));
    memset((void *)cmd_table, 0, AHCI_CMD_TABLE_SIZE);

    u32 prdt_count = 1;
    cmd_header->dw0 = (5u << 0) | (prdt_count << 16);
    if (write) {
        cmd_header->dw0 |= (1u << 6);
    }
    cmd_header->prdbc = 0;

    paddr_t cmd_table_slot_phys = state->cmd_table_phys + slot * AHCI_CMD_TABLE_SIZE;
    cmd_header->ctba = (u32)(cmd_table_slot_phys & 0xFFFFFFFFULL);
    cmd_header->ctbau = (u32)(cmd_table_slot_phys >> 32);

    paddr_t buf_phys = virt_to_phys(buf);
    if (buf_phys == 0) {
        return false;
    }

    cmd_table->prdt[0].dba = (u32)(buf_phys & 0xFFFFFFFFULL);
    cmd_table->prdt[0].dbau = (u32)(buf_phys >> 32);
    cmd_table->prdt[0].dbc = (u32)((buf_bytes - 1) & AHCI_PRDT_BYTE_COUNT_MAX) | (1u << 31);

    volatile u8 *cfis = (volatile u8 *)cmd_table->cfis;
    cfis[0] = AHCI_FIS_TYPE_REG_H2D;
    cfis[1] = 1u << 7; 
    cfis[2] = command;
    cfis[3] = 0;

    cfis[4] = (u8)(lba & 0xFF);
    cfis[5] = (u8)((lba >> 8) & 0xFF);
    cfis[6] = (u8)((lba >> 16) & 0xFF);
    cfis[7] = (command == AHCI_CMD_IDENTIFY) ? 0 : 0x40;
    cfis[8] = (u8)((lba >> 24) & 0xFF);
    cfis[9] = (u8)((lba >> 32) & 0xFF);
    cfis[10] = (u8)((lba >> 40) & 0xFF);
    cfis[11] = 0;

    cfis[12] = (u8)(sector_count & 0xFF);
    cfis[13] = (u8)((sector_count >> 8) & 0xFF);
    cfis[14] = 0;
    cfis[15] = 0;

    port->is = (u32)-1; 
    port->ci |= (1u << slot);

    usize attempts = 0;
    while ((port->ci & (1u << slot)) != 0) {
        if (attempts++ > 5000000) {
            return false;
        }
    }

    u32 tfd = port->tfd;
    if (tfd & 0x88u) {
        return false;
    }

    return true;
}

static bool ahci_port_identify(struct ahci_port_state *state, u64 *out_sectors)
{
    u8 identify_buf[AHCI_SECTOR_SIZE];
    memset(identify_buf, 0, sizeof(identify_buf));

    if (!ahci_port_issue_command(state, false, 0, 1, identify_buf, sizeof(identify_buf), AHCI_CMD_IDENTIFY)) {
        return false;
    }

    u16 *words = (u16 *)identify_buf;
    u64 total_sectors = ((u64)words[103] << 48) |
                        ((u64)words[102] << 32) |
                        ((u64)words[101] << 16) |
                        (u64)words[100];
    if (total_sectors == 0) {
        total_sectors = ((u64)words[61] << 16) | (u64)words[60];
    }
    if (total_sectors == 0) {
        return false;
    }

    *out_sectors = total_sectors;
    return true;
}

static isize ahci_block_read(void *ctx, u64 offset, void *buf, usize size)
{
    struct ahci_port_state *state = (struct ahci_port_state *)ctx;
    if (!state || !state->port || size == 0) {
        return -1;
    }
    if ((offset % AHCI_SECTOR_SIZE) != 0 || (size % AHCI_SECTOR_SIZE) != 0) {
        return -1;
    }
    u64 max_bytes = state->sector_count * AHCI_SECTOR_SIZE;
    if (offset >= max_bytes) {
        return 0;
    }
    usize bytes_to_transfer = size;
    if ((u64)bytes_to_transfer > max_bytes - offset) {
        bytes_to_transfer = (usize)(max_bytes - offset);
    }

    u64 sector = offset / AHCI_SECTOR_SIZE;
    u64 remaining = bytes_to_transfer;
    u8 *dest = buf;

    while (remaining > 0) {
        u32 sectors = (remaining / AHCI_SECTOR_SIZE);
        if (sectors == 0) {
            break;
        }
        if (sectors > 256) {
            sectors = 256;
        }

        usize bytes = sectors * AHCI_SECTOR_SIZE;
        if (!ahci_port_issue_command(state, false, sector, sectors, dest, bytes, AHCI_CMD_READ)) {
            return -1;
        }

        dest += bytes;
        sector += sectors;
        remaining -= bytes;
    }

    return (isize)bytes_to_transfer;
}

static isize ahci_block_write(void *ctx, u64 offset, const void *buf, usize size)
{
    struct ahci_port_state *state = (struct ahci_port_state *)ctx;
    if (!state || !state->port || size == 0) {
        return -1;
    }
    if ((offset % AHCI_SECTOR_SIZE) != 0 || (size % AHCI_SECTOR_SIZE) != 0) {
        return -1;
    }
    u64 max_bytes = state->sector_count * AHCI_SECTOR_SIZE;
    if (offset >= max_bytes) {
        return 0;
    }
    usize bytes_to_transfer = size;
    if ((u64)bytes_to_transfer > max_bytes - offset) {
        bytes_to_transfer = (usize)(max_bytes - offset);
    }

    u64 sector = offset / AHCI_SECTOR_SIZE;
    u64 remaining = bytes_to_transfer;
    const u8 *src = buf;

    while (remaining > 0) {
        u32 sectors = (remaining / AHCI_SECTOR_SIZE);
        if (sectors == 0) {
            break;
        }
        if (sectors > 256) {
            sectors = 256;
        }

        usize bytes = sectors * AHCI_SECTOR_SIZE;
        if (!ahci_port_issue_command(state, true, sector, sectors, (void *)src, bytes, AHCI_CMD_WRITE)) {
            return -1;
        }

        src += bytes;
        sector += sectors;
        remaining -= bytes;
    }

    return (isize)bytes_to_transfer;
}

static void ahci_probe(u8 bus, u8 dev, u8 func,
                       u16 vendor, u16 device,
                       u8 class_code, u8 subclass, u8 prog_if)
{
    (void)class_code;
    (void)subclass;
    (void)prog_if;
    printk_level(LOG_INFO, "ahci: probed device at %02x:%02x.%u vendor=0x%04x device=0x%04x\n",
                 bus, dev, func, vendor, device);

    u32 bar0 = pci_config_read32(bus, dev, func, 0x10);
    if ((bar0 & 1) == 1) {
        printk_level(LOG_WARN, "ahci: device at %02x:%02x.%u uses I/O BAR, skipping\n", bus, dev, func);
        return;
    }

    paddr_t phys = (paddr_t)(bar0 & ~0xFULL);
    vaddr_t virt = (vaddr_t)(VULCAN_KERNEL_VBASE + (phys & 0x00000000FFFFFFFFULL));
    paddr_t base_phys = phys & ~0xFFFULL;

    for (int i = 0; i < 2; i++) {
        if (!paging_map_page(virt + i * 0x1000, base_phys + i * 0x1000, PAGE_PRESENT | PAGE_WRITABLE)) {
            printk_level(LOG_WARN, "ahci: failed to map BAR0 phys=0x%llx\n", phys);
            return;
        }
    }

    volatile struct ahci_hba_mem *hba = (volatile struct ahci_hba_mem *)virt;
    printk_level(LOG_INFO, "ahci: HBA CAP=0x%08x VER=0x%08x mapped virt=%p phys=0x%llx\n",
                 hba->cap, hba->vs, (void *)virt, (u64)phys);

    hba->ghc |= (1u << 31);

    u32 pi = hba->pi;
    printk_level(LOG_INFO, "ahci: port implemented mask = 0x%08x\n", pi);

    int sd_index = 0;
    for (int p = 0; p < AHCI_MAX_PORTS; p++) {
        if (!(pi & (1u << p))) {
            continue;
        }

        volatile struct ahci_port *port = &hba->ports[p];
        if (!ahci_port_device_present(port)) {
            continue;
        }

        struct ahci_port_state *state = &ahci_states[p];
        state->port = port;
        state->port = port;
        state->sector_count = 0;

        if (!ahci_port_configure(state)) {
            printk_level(LOG_WARN, "ahci: failed to configure port %d\n", p);
            continue;
        }

        u64 sectors = 0;
        if (!ahci_port_identify(state, &sectors)) {
            printk_level(LOG_WARN, "ahci: failed to identify port %d\n", p);
            continue;
        }

        state->sector_count = sectors;

        char name[8];
        name[0] = 's';
        name[1] = 'd';
        int n = sd_index;
        int pos = 2;
        if (n == 0) {
            name[pos++] = '0';
        } else {
            char rev[4];
            int ri = 0;
            while (n > 0 && ri < 4) {
                rev[ri++] = (char)('0' + (n % 10));
                n /= 10;
            }
            for (int i = ri - 1; i >= 0; i--) {
                name[pos++] = rev[i];
            }
        }
        name[pos] = '\0';

        int name_idx = 0;
        for (; name[name_idx]; name_idx++) {
            state->name[name_idx] = name[name_idx];
        }
        state->name[name_idx] = '\0';

        u64 size_bytes = sectors * AHCI_SECTOR_SIZE;
        if (!block_device_register(name, size_bytes, state, ahci_block_read, ahci_block_write)) {
            printk_level(LOG_WARN, "ahci: failed to register block device %s for port %d\n", name, p);
            continue;
        }
        printk_level(LOG_INFO, "ahci: registered block device %s size=%llu for port %d\n",
                     name, size_bytes, p);
        sd_index++;
    }
}

bool ahci_driver_register(void)
{
    static const struct pci_driver drv = {
        .name = "ahci",
        .class_code = 0x01,
        .subclass = 0x06,
        .prog_if = 0xFF,
        .vendor = 0xFFFF,
        .device = 0xFFFF,
        .init = ahci_probe,
    };

    return pci_register_driver(&drv);
}
